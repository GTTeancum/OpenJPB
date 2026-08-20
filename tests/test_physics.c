#include "jpb/anim.h"
#include "jpb/bmd.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/cube.h"
#include "jpb/effects.h"
#include "jpb/flex.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include "jpb/alloc.h"
#include "jpb/model.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/vehicle.h"
#include "jpb/world.h"
#include "jpb/wrender.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int32_t pack_solid_vector(int x, int y, int z)
{
    uint32_t packed =
        ((uint32_t)x & UINT32_C(0x3ff)) |
        (((uint32_t)y & UINT32_C(0x3ff)) << 10) |
        (((uint32_t)z & UINT32_C(0x3ff)) << 20);

    return (int32_t)packed;
}

typedef struct GeometryResolverFixture {
    const geomData *geometry;
    int pointerType;
    void *stream;
    int callCount;
} GeometryResolverFixture;

static void *test_geometry_stream_resolver(
    const geomData *geometry,
    int pointer_type,
    void *user_data)
{
    GeometryResolverFixture *fixture =
        (GeometryResolverFixture *)user_data;

    ++fixture->callCount;
    if (geometry == fixture->geometry &&
        pointer_type == fixture->pointerType) {
        return fixture->stream;
    }
    return NULL;
}

static int test_geometry_stream_resolution(void)
{
    geomData geometry;
    GeometryResolverFixture fixture;
    uint32_t registered_vertex = UINT32_C(0x12345678);
    uint32_t owned_normal = UINT32_C(0x87654321);

    memset(&geometry, 0, sizeof(geometry));
    memset(&fixture, 0, sizeof(fixture));
    pointerRegistry_Reset();
    geometry.pVertex = addPtr(
        &registered_vertex, JPB_POINTER_ARRAY_VERTEX);
    CHECK(geometry.pVertex >= 0);
    CHECK(jpb_PhysicsResolveGeometryStream(
              &geometry, JPB_POINTER_ARRAY_VERTEX) ==
          &registered_vertex);

    fixture.geometry = &geometry;
    fixture.pointerType = JPB_POINTER_ARRAY_NORMAL;
    fixture.stream = &owned_normal;
    jpb_PhysicsSetGeometryStreamResolver(
        test_geometry_stream_resolver, &fixture);
    CHECK(jpb_PhysicsResolveGeometryStream(
              &geometry, JPB_POINTER_ARRAY_NORMAL) ==
          &owned_normal);
    CHECK(jpb_PhysicsResolveGeometryStream(
              &geometry, JPB_POINTER_ARRAY_VERTEX) ==
          &registered_vertex);
    CHECK(fixture.callCount == 2);
    CHECK(jpb_PhysicsResolveGeometryStream(
              &geometry, JPB_POINTER_ARRAY_ENEMY) == NULL);
    jpb_PhysicsSetGeometryStreamResolver(NULL, NULL);
    pointerRegistry_Reset();
    return 0;
}

static void connect_actor(
    objectRoot *actor, sceneObject *scene, physicsObject *physics)
{
    memset(actor, 0, sizeof(*actor));
    memset(scene, 0, sizeof(*scene));
    memset(physics, 0, sizeof(*physics));
    actor->pParent = &scene->sceneRoot;
    scene->pPhysics = &physics->physicsRoot;
}

static void connect_complete_actor(
    objectRoot *actor,
    sceneObject *scene,
    modelObject *model,
    physicsObject *physics,
    animObject *animation,
    playerObject *player);

static int test_trajectory_callback(
    int32_t *arguments, playerObject *player)
{
    (void)arguments;
    (void)player;
    return 0;
}

typedef struct TestSoundStopCapture {
    uint16_t handles[2];
    int count;
} TestSoundStopCapture;

static void test_sound_stop_hook(
    uint16_t handle, void *user_data)
{
    TestSoundStopCapture *capture =
        (TestSoundStopCapture *)user_data;

    if (capture->count < 2) {
        capture->handles[capture->count] = handle;
    }
    ++capture->count;
}

static int test_public_state_conversion(void)
{
    physicsObject physics;

    memset(&physics, 0x5a, sizeof(physics));
    physics.angle.vx = 0x12345;
    physics.angle.vy = -2;
    physics.angle.vz = 0x7fff;
    physics.pos.vx = 12.75f;
    physics.pos.vy = -8.75f;
    physics.pos.vz = 32767.9f;
    physics.mov.vx = 1.9f;
    physics.mov.vy = -2.9f;
    physics.mov.vz = 3.1f;
    UpdatePublicVars(&physics);

    CHECK(physics.svangle.vx == (int16_t)0x2345);
    CHECK(physics.svangle.vy == -2);
    CHECK(physics.svangle.vz == 0x7fff);
    CHECK(physics.svpos.vx == 12);
    CHECK(physics.svpos.vy == -8);
    CHECK(physics.svpos.vz == 32767);
    CHECK(physics.svmov.vx == 1);
    CHECK(physics.svmov.vy == -2);
    CHECK(physics.svmov.vz == 3);
    CHECK(physics.vpos.vx == 12);
    CHECK(physics.vpos.vy == -8);
    CHECK(physics.vpos.vz == 32767);
    CHECK(physics.vpos.pad == 0x5a5a5a5a);
    return 0;
}

static int test_update_scene_object(void)
{
    physicsObject *physics;
    physicsObject *target_physics;
    sceneObject *scene;
    sceneObject *target_scene;
    playerObject *player;
    playerObject *target;
    int expected_facing;

    CHECK(jpb_PhysicsUpdateSceneObject(NULL) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    memset(gaPlayerData, 0, sizeof(gaPlayerData));

    physics = &maPhysicsData[0];
    target_physics = &maPhysicsData[1];
    scene = &maSceneData[0];
    target_scene = &maSceneData[1];
    player = &gaPlayerData[0];
    target = &gaPlayerData[1];

    physics->physicsRoot.objectID = 0;
    physics->physicsRoot.pParent = &scene->sceneRoot;
    scene->pPhysics = &physics->physicsRoot;
    scene->pPlayer = &player->playerRoot;
    player->playerRoot.objectID = 0;
    player->playerRoot.pParent = &scene->sceneRoot;
    player->target = target;

    target_physics->physicsRoot.objectID = 1;
    target_physics->physicsRoot.pParent =
        &target_scene->sceneRoot;
    target_scene->pPhysics =
        &target_physics->physicsRoot;
    target_scene->pPlayer = &target->playerRoot;
    target->playerRoot.objectID = 1;
    target->playerRoot.pParent =
        &target_scene->sceneRoot;

    physics->vpos.vx = -25;
    physics->vpos.vz = 40;
    target_physics->vpos.vx = 75;
    target_physics->vpos.vz = -60;
    physics->pos.vx = 12.75f;
    physics->pos.vy = 100.5f;
    physics->pos.vz = -30.25f;
    physics->angle.vx = 11;
    physics->angle.vy = 22;
    physics->angle.vz = 33;
    physics->airGround = -100.0f;
    physics->validairground = -500.0f;
    player->pFlags = UINT32_C(0x00400000);
    expected_facing = ratan2(100, -100) & 0x0fff;

    CHECK(jpb_PhysicsUpdateSceneObject(physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics->angle.vy == expected_facing);
    CHECK(physics->vpos.vx == 12);
    CHECK(physics->vpos.vy == 100);
    CHECK(physics->vpos.vz == -30);
    CHECK(scene->v3WorldAngle.vx == 11);
    CHECK(scene->v3WorldAngle.vy ==
          (int16_t)expected_facing);
    CHECK(scene->v3WorldAngle.vz == 33);
    CHECK(scene->v3WorldPosition.vx == 12);
    CHECK(scene->v3WorldPosition.vy == 100);
    CHECK(scene->v3WorldPosition.vz == -30);
    CHECK(physics->validairground == -100.0f);

    player->pFlags = UINT32_C(0x00080000);
    physics->face.vx = 101;
    physics->face.vy = 202;
    physics->face.vz = 303;
    physics->pos.vy = 700.0f;
    physics->airGround = 100.0f;
    physics->validairground = 55.0f;

    CHECK(jpb_PhysicsUpdateSceneObject(physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(scene->v3WorldAngle.vx == 101);
    CHECK(scene->v3WorldAngle.vy == 202);
    CHECK(scene->v3WorldAngle.vz == 303);
    CHECK(physics->validairground == 55.0f);

    physics->physicsRoot.pParent = NULL;
    CHECK(jpb_PhysicsUpdateSceneObject(physics) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);
    return 0;
}

static int test_set_recoil(void)
{
    playerObject player = {0};
    playerObject target = {0};
    sceneObject scene = {0};
    sceneObject target_scene = {0};
    physicsObject physics = {0};
    physicsObject target_physics = {0};
    int expected_facing;

    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pPhysics = &physics.physicsRoot;
    player.target = &target;
    target.playerRoot.pParent = &target_scene.sceneRoot;
    target_scene.pPhysics = &target_physics.physicsRoot;
    physics.pos.vx = -25.0f;
    physics.pos.vz = 40.0f;
    target_physics.pos.vx = 75.0f;
    target_physics.pos.vz = -60.0f;
    physics.constmov.vz = 10.0f;
    expected_facing = ratan2(100, -100) & 0x0fff;

    physics_gSetRecoil(&player, 12, 7, 0);
    CHECK(physics.angle.vy == expected_facing);
    CHECK(physics.constmov.vz == 22.0f);
    CHECK(physics.accel.vz == 7.0f);

    physics.angle.vy = 123;
    physics_gSetRecoil(&player, 5, -1, 1);
    CHECK(physics.angle.vy == 123);
    CHECK(physics.constmov.vz == 17.0f);
    CHECK(physics.accel.vz == 0.0f);

    player.target = NULL;
    physics_gSetRecoil(&player, 3, 2, 0);
    CHECK(physics.angle.vy == 0);
    CHECK(physics.constmov.vz == 20.0f);
    CHECK(physics.accel.vz == 2.0f);

    physics.flags = UINT32_C(0x00400000);
    physics.angle.vy = 321;
    physics_gSetRecoil(&player, 99, 99, 0);
    CHECK(physics.angle.vy == 321);
    CHECK(physics.constmov.vz == 20.0f);
    CHECK(physics.accel.vz == 2.0f);
    return 0;
}

static int test_driver_state_sync(void)
{
    physicsObject *driver;
    physicsObject *driven;
    playerObject *player;
    int32_t cube = 1;
    int32_t entry = 2;
    int32_t poly = 3;

    physics_gInitObjects(0);
    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    CHECK(jpb_PhysicsSyncDriverState(-1) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);
    CHECK(jpb_PhysicsSyncDriverState(2) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);

    player = &gaPlayerData[0];
    driver = &maPhysicsData[0];
    driven = &maPhysicsData[5];
    player->playerRoot.objectID = 0;
    driver->flags = UINT32_C(0x00000025);
    driven->pos.vx = 10.0f;
    driven->pos.vy = 20.0f;
    driven->pos.vz = 30.0f;
    driven->angle.vx = 100;
    driven->angle.vy = 200;
    driven->angle.vz = 300;
    driven->mov.vx = 1.0f;
    driven->mov.vy = 2.0f;
    driven->mov.vz = 3.0f;
    driven->mapinfo.cube = &cube;
    driven->mapinfo.entry = &entry;
    driven->mapinfo.poly = &poly;

    CHECK(jpb_PhysicsSyncDriverState(0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(driver->pos.vx == 10.0f);
    CHECK(driver->pos.vy == 20.0f);
    CHECK(driver->pos.vz == 30.0f);
    CHECK(driver->angle.vx == 100);
    CHECK(driver->angle.vy == 200);
    CHECK(driver->angle.vz == 300);
    CHECK(driver->mov.vx == 1.0f);
    CHECK(driver->mov.vy == 2.0f);
    CHECK(driver->mov.vz == 3.0f);
    CHECK(driver->mapinfo.cube == &cube);
    CHECK(driver->mapinfo.entry == &entry);
    CHECK(driver->mapinfo.poly == &poly);

    player->pFlags = UINT32_C(0x00000200);
    driver->pos.vx = -1.0f;
    driven->pos.vx = 99.0f;
    CHECK(jpb_PhysicsSyncDriverState(0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(driver->pos.vx == -1.0f);
    return 0;
}

static int test_root_accessors_and_facing_lock(void)
{
    objectRoot actor;
    sceneObject scene;
    physicsObject physics;

    connect_actor(&actor, &scene, &physics);
    CHECK(physics_gGetPosition(NULL) == NULL);
    CHECK(physics_gGetPosition(&actor) == &physics.vpos);
    physics.vpos.vx = 41;
    CHECK(physics_gGetPosition(&actor)->vx == 41);

    physics_gSetConstantVector(&actor, 1.25f, -2.5f, 3.75f);
    CHECK(physics_gGetConstantVector(&actor) == &physics.constmov);
    CHECK(physics.constmov.vx == 1.25f);
    CHECK(physics.constmov.vy == -2.5f);
    CHECK(physics.constmov.vz == 3.75f);
    physics_gClrConstantVector(&actor);
    CHECK(physics.constmov.vx == 0.0f);
    CHECK(physics.constmov.vy == 0.0f);
    CHECK(physics.constmov.vz == 0.0f);

    gSCENE_READY = 0;
    physics.flags = 0x00400000u;
    physics_gSetFacing(&actor, 100);
    CHECK(physics_gGetFacing(&actor) == 100);
    gSCENE_READY = 1;
    physics_gSetFacing(&actor, 200);
    CHECK(physics_gGetFacing(&actor) == 100);
    physics.flags = 0;
    physics_gSetFacing(&actor, 300);
    CHECK(physics_gGetFacing(&actor) == 300);

    scene.pPhysics = NULL;
    CHECK(physics_gGetPosition(&actor) == NULL);
    CHECK(physics_gGetConstantVector(&actor) == NULL);
    physics_gClrConstantVector(&actor);
    CHECK(physics_gGetFacing(&actor) == 0);
    actor.pParent = NULL;
    CHECK(physics_gGetPosition(&actor) == NULL);
    CHECK(physics_gGetFacing(&actor) == 0);
    gSCENE_READY = 0;
    return 0;
}

static int test_pool_initialization(void)
{
    physicsObject before;
    int index;

    memset(maPhysicsData, 0xa5, sizeof(maPhysicsData));
    numsolids = 99;
    physics_gInitObjects(0);
    CHECK(numsolids == 0);
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        physicsObject *physics = &maPhysicsData[index];

        CHECK(physics->physicsRoot.pParent == NULL);
        CHECK(physics->physicsRoot.flags == 0);
        CHECK(physics->physicsRoot.objectID == -1);
        CHECK(memcmp(
                  physics->physicsRoot.objectName,
                  "PHYSICS",
                  sizeof("PHYSICS")) == 0);
        CHECK(physics->matrix.m[0][0] == 0.0f);
        CHECK(physics->angle.vx == 0);
        CHECK(physics->airspeed == 0);
        CHECK(physics->turnspeed == 0x500);
        CHECK(physics->radius == 0x36);
        CHECK(physics->mass == 0x800);
        CHECK(physics->height == 0xdc);
        CHECK(physics->noncollideframes == 0);
        CHECK(physics->solid == NULL);
        CHECK(physics->uservector.vz == 0);
    }

    memset(&before, 0x6b, sizeof(before));
    maPhysicsData[9] = before;
    maPhysicsData[10] = before;
    physics_gInitObjects(10);
    CHECK(memcmp(&maPhysicsData[9], &before, sizeof(before)) == 0);
    CHECK(maPhysicsData[10].physicsRoot.objectID == -1);
    CHECK(maPhysicsData[10].turnspeed == 0x500);

    before = maPhysicsData[0];
    numsolids = 55;
    physics_gInitObjects(-1);
    CHECK(memcmp(&maPhysicsData[0], &before, sizeof(before)) == 0);
    CHECK(numsolids == 0);
    return 0;
}

static int test_pool_allocation_and_cleanup(void)
{
    physicsObject *physics;
    _solid *old_solid;
    _svector *old_coords;
    int index;

    meminit();
    physics_gInitObjects(0);
    physics = physics_gGetNewObject(-1);
    CHECK(physics == &maPhysicsData[0]);
    CHECK(physics->physicsRoot.objectID == 0);
    CHECK(physics_gGetNewObject(-1) == &maPhysicsData[1]);

    physics = physics_gGetNewObject(5);
    CHECK(physics == &maPhysicsData[5]);
    CHECK(physics->physicsRoot.objectID == 5);
    old_solid = (_solid *)memalloc(sizeof(*old_solid));
    old_coords = (_svector *)memalloc(sizeof(*old_coords) * 4);
    CHECK(old_solid != NULL);
    CHECK(old_coords != NULL);
    memset(old_solid, 0, sizeof(*old_solid));
    old_solid->coords = old_coords;
    physics->solid = old_solid;
    physics->flags = UINT32_MAX;
    physics->lastpolyhit = (int32_t *)(uintptr_t)1;
    memset(physics->userdata, 0x7f, sizeof(physics->userdata));
    memset(&physics->uservector, 0x7f, sizeof(physics->uservector));

    CHECK(physics_gGetNewObject(5) == physics);
    CHECK(physics->physicsRoot.objectID == 5);
    CHECK(physics->solid == NULL);
    CHECK(physics->flags == 0);
    CHECK(physics->lastpolyhit == NULL);
    CHECK(physics->userdata[0] == 0);
    CHECK(physics->userdata[1] == 0);
    CHECK(physics->userdata[2] == 0);
    CHECK(physics->uservector.vx == 0);
    CHECK(physics->uservector.vy == 0);
    CHECK(physics->uservector.vz == 0);
    CHECK(physics->uservector.pad == 0);
    CHECK(memalloc(sizeof(*old_solid)) == old_solid);

    CHECK(physics_gGetNewObject(JPB_PHYSICS_CAPACITY) == NULL);
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        maPhysicsData[index].physicsRoot.objectID = index;
    }
    CHECK(physics_gGetNewObject(-1) == NULL);
    return 0;
}

static int test_solid_builder_geometry_and_ownership(void)
{
    sceneObject *other_scene;
    sceneObject *solid_scene;
    physicsObject *other;
    physicsObject *candidate;
    modelObject model;
    playerObject player;
    wsl_ENEMY enemy;
    Mnode node;
    geomData geometry;
    int32_t packed_vertices[3];
    int32_t packed_normals[1];
    _solid *solid;
    _solid *old_solid;
    _svector *old_coords;
    uintptr_t old_solid_address;
    void *reused;

    meminit();
    pointerRegistry_Reset();
    coll_ResetCollisionSystem();
    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    memset(&model, 0, sizeof(model));
    memset(&player, 0, sizeof(player));
    memset(&enemy, 0, sizeof(enemy));
    memset(&node, 0, sizeof(node));
    memset(&geometry, 0, sizeof(geometry));

    other_scene = &maSceneData[0];
    solid_scene = &maSceneData[2];
    other = &maPhysicsData[0];
    candidate = &maPhysicsData[2];

    other_scene->sceneRoot.objectID = 0;
    other_scene->pScene = &other_scene->sceneRoot;
    other_scene->pPhysics = &other->physicsRoot;
    other->physicsRoot.objectID = 0;
    other->physicsRoot.pParent = &other_scene->sceneRoot;
    other->pos.vx = 100.0f;

    solid_scene->sceneRoot.objectID = 2;
    solid_scene->pScene = &solid_scene->sceneRoot;
    solid_scene->pModel = &model.modelRoot;
    solid_scene->pPhysics = &candidate->physicsRoot;
    solid_scene->pPlayer = &player.playerRoot;
    candidate->physicsRoot.objectID = 2;
    candidate->physicsRoot.pParent = &solid_scene->sceneRoot;
    candidate->height = 20;
    model.modelRoot.objectID = 2;
    model.modelRoot.pParent = &solid_scene->sceneRoot;
    model.flags = UINT32_C(2);
    model.v3Scale.vx = JPB_FIXED_ONE;
    model.v3Scale.vy = JPB_FIXED_ONE;
    model.v3Scale.vz = JPB_FIXED_ONE;
    player.playerRoot.objectID = 2;
    player.playerRoot.pParent = &solid_scene->sceneRoot;
    player.pEnemy = &enemy;
    enemy.enemyFlags = UINT32_C(0xc000);

    packed_vertices[0] = pack_solid_vector(1, 2, 3);
    packed_vertices[1] = pack_solid_vector(-4, 5, -6);
    packed_vertices[2] = pack_solid_vector(7, -8, 9);
    packed_normals[0] = pack_solid_vector(0, 1, 0);
    geometry.numVerts = 1;
    geometry.numFaces = 1;
    geometry.pVertex = addPtr(packed_vertices, 0);
    geometry.pNormal = addPtr(packed_normals, 1);
    CHECK(geometry.pVertex >= 0);
    CHECK(geometry.pNormal >= 0);
    node.id = NODE_DYNAMIC;
    node.pGeomData = &geometry;
    node.v3RotCenter.vx = 10;
    node.v3RotCenter.vy = 20;
    node.v3RotCenter.vz = 30;
    coll_gRegisterNode(2, &node);

    gSCENE_READY = 1;
    jpb_PhysicsBuildSolids();
    CHECK(numsolids == 1);
    solid = candidate->solid;
    CHECK(solid != NULL);
    CHECK(solid->object == solid_scene);
    CHECK(solid->modelnode == &node);
    CHECK(solid->geometry == &geometry);
    CHECK(solid->model == &model);
    CHECK(solid->physics == candidate);
    CHECK(solid->node == 0);
    CHECK(solid->flags == UINT32_C(3));
    CHECK((candidate->flags & UINT32_C(0x400)) != 0);
    CHECK(solid->coords != NULL);
    CHECK(solid->normals == solid->coords + 3);
    CHECK(solid->coords[0].vx == 11);
    CHECK(solid->coords[0].vy == 22);
    CHECK(solid->coords[0].vz == 33);
    CHECK(solid->coords[1].vx == 6);
    CHECK(solid->coords[1].vy == 25);
    CHECK(solid->coords[1].vz == 24);
    CHECK(solid->coords[2].vx == 17);
    CHECK(solid->coords[2].vy == 12);
    CHECK(solid->coords[2].vz == 39);
    CHECK(solid->normals[0].vx == 0);
    CHECK(solid->normals[0].vy == 8);
    CHECK(solid->normals[0].vz == 0);
    CHECK(solid->scale.vx == JPB_FIXED_ONE);
    CHECK(solid->scale.vy == JPB_FIXED_ONE);
    CHECK(solid->scale.vz == JPB_FIXED_ONE);

    old_solid = solid;
    old_solid_address = (uintptr_t)(void *)old_solid;
    old_coords = solid->coords;
    node.v3RotCenter.vx = 20;
    jpb_PhysicsBuildSolids();
    CHECK(numsolids == 1);
    CHECK(candidate->solid == old_solid);
    CHECK(candidate->solid->coords == old_coords);
    CHECK(candidate->solid->coords[0].vx == 21);

    other->pos.vx = 10000.0f;
    jpb_PhysicsBuildSolids();
    CHECK(numsolids == 0);
    CHECK(candidate->solid == NULL);
    reused = memalloc((unsigned)sizeof(*old_solid));
    CHECK((uintptr_t)reused == old_solid_address);
    memfree(reused);

    gSCENE_READY = 0;
    pointerRegistry_Reset();
    coll_ResetCollisionSystem();
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    return 0;
}

static int test_process_physics_objects_scheduler(void)
{
    static WorldData world;
    objectRoot actor0;
    objectRoot actor1;
    modelObject model0;
    animObject animation0;
    physicsObject *physics0 = &maPhysicsData[0];
    physicsObject *physics1 = &maPhysicsData[1];
    playerObject *player0 = &gaPlayerData[0];
    playerObject *player1 = &gaPlayerData[1];
    sceneObject *scene0 = &maSceneData[0];
    sceneObject *scene1 = &maSceneData[1];
    WorldData *old_world = gpWorld;
    gamestruct old_game = GameStruct;
    optionstruct old_options = OptionStruct;
    FVECTOR4 old_collision[6];
    FVECTOR4 old_clipping[6];
    MATRIX old_matrix = *jpb_WRenderCurrentMatrix();
    int old_camera_type = camera_GetCurrentCameraType();
    int old_fixed_frame_rate = gGlobalFrameRate;
    float old_frame_rate = fGlobalFrameRate;
    char old_level = LevelSelect;
    uint8_t old_pause_delay = initialLevelPauseDelay;

    memcpy(
        old_collision,
        collisionfrustrum,
        sizeof(old_collision));
    memcpy(
        old_clipping,
        clippingfrustrum,
        sizeof(old_clipping));
    memset(&world, 0, sizeof(world));
    memset(&actor1, 0, sizeof(actor1));
    memset(collisionfrustrum, 0, sizeof(collisionfrustrum));
    memset(clippingfrustrum, 0, sizeof(clippingfrustrum));
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    player_gInitPlayers(0);

    connect_complete_actor(
        &actor0,
        scene0,
        &model0,
        physics0,
        &animation0,
        player0);
    actor1.objectID = 1;
    obj_gSetChildObject(scene1, &actor1, 0);
    obj_gSetChildObject(scene1, &player1->playerRoot, 4);
    physics0->movemode = MOVE_FLY;
    physics0->pos.vx = 10.0f;
    physics0->pos.vy = 20.0f;
    physics0->pos.vz = 30.0f;
    physics0->constmov.vx = 2.0f;
    physics0->constmov.vy = 4.0f;
    physics0->constmov.vz = 6.0f;
    physics0->validairground = 12.0f;
    physics1->validairground = 20.0f;
    gpWorld = &world;
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    LevelSelect = 0;
    OptionStruct.AIDebug = 0;
    initialLevelPauseDelay = 0;
    GameStruct.GameState = UINT32_C(0x02000000);
    camera_SetCurrentCameraType(1);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);

    gSCENE_READY = 0;
    ProcessPhysicsObjects();
    CHECK(physics0->pos.vx == 10.0f);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);

    gSCENE_READY = 1;
    ProcessPhysicsObjects();
    CHECK(physics0->pos.vx == 11.0f);
    CHECK(physics0->pos.vy == 22.0f);
    CHECK(physics0->pos.vz == 33.0f);
    CHECK(world.p0location.vx == 11);
    CHECK(world.p0location.vy == 22);
    CHECK(world.p0location.vz == 33);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);
    CHECK(memcmp(
              jpb_WRenderCurrentMatrix(),
              &old_matrix,
              sizeof(old_matrix)) == 0);

    initialLevelPauseDelay = 2;
    ProcessPhysicsObjects();
    CHECK(physics0->pos.vx == 11.0f);
    CHECK(physics0->pos.vy == 22.0f);
    CHECK(physics0->pos.vz == 33.0f);

    GameStruct.GameState = 0;
    ProcessPhysicsObjects();
    CHECK(physics0->pos.vx == 12.0f);
    CHECK(physics0->pos.vy == 24.0f);
    CHECK(physics0->pos.vz == 36.0f);
    CHECK(world.p0location.vx == 12);
    CHECK(world.p0location.vy == 24);
    CHECK(world.p0location.vz == 36);
    CHECK(game_gIsGameFlags(UINT32_C(0x02000000)) == 0);

    GameStruct.aCharacterData[0].Energy = 75;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    GameStruct.aCharacterData[1].Energy = 60;
    GameStruct.aCharacterData[1].MaxEnergy = 100;
    GameStruct.GameState = UINT32_C(0x02000000);
    actor0.flags = 0;
    actor1.flags = 0;
    physics0->flags = UINT32_C(0x000000c0);
    physics1->flags = UINT32_C(0x123456ff);
    maRange[1][4] = 4.0f;
    maRange[1][5] = 5.0f;
    jpb_PhysicsSetStreetsEndingCountdown(gGlobalFrameRate);
    ProcessPhysicsObjects();
    CHECK(jpb_PhysicsGetStreetsEndingCountdown() == 0);
    CHECK(GameStruct.aCharacterData[0].Energy == 0);
    CHECK(GameStruct.aCharacterData[1].Energy == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x00000060)) ==
          UINT32_C(0x00000060));
    CHECK((actor0.flags & UINT32_C(0x00000020)) != 0);
    CHECK((actor1.flags & UINT32_C(0x00000020)) != 0);
    CHECK((physics0->flags & UINT32_C(0x000000ff)) ==
          UINT32_C(0x00000040));
    CHECK(
        physics1->flags ==
        (UINT32_C(0x123456ff) & UINT32_C(0xffffff40)));
    CHECK(maRange[1][4] == 0.0f);
    CHECK(maRange[1][5] == 0.0f);
    CHECK(jpb_WRenderMatrixStackLevel() == 0);

    gSCENE_READY = 0;
    jpb_PhysicsSetStreetsEndingCountdown(0);
    gpWorld = old_world;
    GameStruct = old_game;
    OptionStruct = old_options;
    gGlobalFrameRate = old_fixed_frame_rate;
    fGlobalFrameRate = old_frame_rate;
    LevelSelect = old_level;
    initialLevelPauseDelay = old_pause_delay;
    memcpy(
        collisionfrustrum,
        old_collision,
        sizeof(old_collision));
    memcpy(
        clippingfrustrum,
        old_clipping,
        sizeof(old_clipping));
    camera_SetCurrentCameraType(old_camera_type);
    *jpb_WRenderCurrentMatrix() = old_matrix;
    player_gInitPlayers(0);
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    return 0;
}

static int test_facing_modification_publication(void)
{
    objectRoot actor;
    sceneObject owner;
    physicsObject *physics;

    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    memset(&actor, 0, sizeof(actor));
    memset(&owner, 0, sizeof(owner));
    physics = &maPhysicsData[0];
    actor.objectID = 0;
    actor.pParent = &owner.sceneRoot;
    owner.pPhysics = &physics->physicsRoot;
    physics->physicsRoot.objectID = 0;
    physics->physicsRoot.pParent = &owner.sceneRoot;
    physics->angle.vx = 0x12345;
    physics->angle.vy = 4000;
    physics->angle.vz = -2;
    physics->pos.vx = 12.75f;
    physics->pos.vy = -8.75f;
    physics->pos.vz = 30.5f;
    gSCENE_READY = 0;

    physics_gModFacing(&actor, 200);
    CHECK(physics->angle.vy == 104);
    CHECK((physics->flags & 0x00001000u) != 0);
    CHECK(maSceneData[0].v3WorldAngle.vx ==
          (int16_t)0x2345);
    CHECK(maSceneData[0].v3WorldAngle.vy == 104);
    CHECK(maSceneData[0].v3WorldAngle.vz == -2);
    CHECK(maSceneData[0].v3WorldPosition.vx == 12);
    CHECK(maSceneData[0].v3WorldPosition.vy == -8);
    CHECK(maSceneData[0].v3WorldPosition.vz == 30);

    gSCENE_READY = 1;
    physics->flags |= 0x00400000u;
    physics_gModFacing(&actor, 300);
    CHECK(physics->angle.vy == 104);
    gSCENE_READY = 0;
    return 0;
}

static int test_turn_to_face(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    gSCENE_READY = 0;
    gGlobalFrameRate = 2048;
    physics.angle.vy = 0;
    physics_gTurnToFace(&actor, 1024, 2);
    CHECK(physics.angle.vy == 256);
    CHECK((physics.flags & 0x00001000u) != 0);

    physics.flags = 0x00000100u;
    physics.angle.vy = 0;
    physics_gTurnToFace(&actor, 100, 2);
    CHECK(physics.angle.vy == 4);

    physics.flags = 0;
    physics.movemode = MOVE_HOVER3D;
    physics.angle.vy = 0;
    gGlobalFrameRate = JPB_FIXED_ONE;
    physics_gTurnToFace(&actor, 1024, 2);
    CHECK(physics.angle.vy == 96);

    player.playerID = 0x43;
    physics.angle.vy = 0;
    physics_gTurnToFace(&actor, 1024, 2);
    CHECK(physics.angle.vy == 12);

    gSCENE_READY = 1;
    physics.flags = 0x00400000u;
    physics.angle.vy = 300;
    physics_gTurnToFace(&actor, 900, 2);
    CHECK(physics.angle.vy == 300);
    gSCENE_READY = 0;
    return 0;
}

static int test_turn_to_attack(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    _animTemplate template_data;
    animListNode current_sequence;
    Motion motion;

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    memset(&template_data, 0, sizeof(template_data));
    memset(&current_sequence, 0, sizeof(current_sequence));
    memset(&motion, 0, sizeof(motion));
    template_data.Lframe = 2;
    current_sequence.pAnimTemplate = &template_data;
    animation.pCurrentAnimSeq = &current_sequence;
    animation.pMotion = &motion;
    animation.animFrameIndex = 0;
    physics.vmov.vx = 1;
    physics.angle.vy = 0;
    gSCENE_READY = 0;
    gGlobalFrameRate = 2048;

    physics_gTurnToAttack(&actor, 1024, 2);
    CHECK(physics.angle.vy == 256);
    CHECK((physics.flags & 0x00001000u) != 0);

    physics.flags = 0;
    animation.tweenFramesLeft = 1;
    physics_gTurnToAttack(&actor, 2048, 2);
    CHECK(physics.angle.vy == 256);

    animation.tweenFramesLeft = 0;
    physics.vmov.vx = 0;
    physics_gTurnToAttack(&actor, 2048, 2);
    CHECK(physics.angle.vy == 256);
    return 0;
}

static void connect_complete_actor(
    objectRoot *actor,
    sceneObject *scene,
    modelObject *model,
    physicsObject *physics,
    animObject *animation,
    playerObject *player)
{
    memset(actor, 0, sizeof(*actor));
    memset(scene, 0, sizeof(*scene));
    memset(model, 0, sizeof(*model));
    memset(physics, 0, sizeof(*physics));
    memset(animation, 0, sizeof(*animation));
    memset(player, 0, sizeof(*player));
    actor->objectID = 0;
    model->modelRoot.objectID = 0;
    physics->physicsRoot.objectID = 0;
    animation->animRoot.objectID = 0;
    player->playerRoot.objectID = 0;
    model->v3Scale.vx = JPB_FIXED_ONE;
    model->v3Scale.vy = JPB_FIXED_ONE;
    model->v3Scale.vz = JPB_FIXED_ONE;
    obj_gSetChildObject(scene, actor, 0);
    obj_gSetChildObject(scene, &model->modelRoot, 1);
    obj_gSetChildObject(scene, &physics->physicsRoot, 2);
    obj_gSetChildObject(scene, &animation->animRoot, 3);
    obj_gSetChildObject(scene, &player->playerRoot, 4);
}

static int test_normal_movement_and_no_contact_commit(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    _solid solid;

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    fGlobalFrameRate = 0.5f;
    globalgravity.vx = 0.0f;
    globalgravity.vy = -11.0f;
    globalgravity.vz = 0.0f;

    physics.constmov.vz = 100.0f;
    physics.accel.vz = 10.0f;
    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.constmov.vz == 90.0f);
    CHECK(physics.currentmov.vz == 90.0f);
    CHECK(physics.mov.vx == 0.0f);
    CHECK(physics.mov.vz == 45.0f);
    CHECK((physics.flags & 0x00004800u) == 0x00004800u);
    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_ALREADY_PROCESSED);

    CHECK(jpb_PhysicsMoveNoContact(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.pos.vz == 45.0f);
    CHECK((physics.flags & 0x00001800u) == 0);

    jpb_PhysicsBeginObjectFrame(&physics);
    physics.constmov.vz = 100.0f;
    physics.accel.vz = 0.0f;
    physics.angle.vy = 1024;
    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx - 50.0f) < 0.001f);
    CHECK(fabsf(physics.mov.vz) < 0.001f);

    jpb_PhysicsBeginObjectFrame(&physics);
    physics.constmov.vx = 0.0f;
    physics.constmov.vy = 0.0f;
    physics.constmov.vz = 0.0f;
    physics.angle.vy = 0;
    physics.airmov.vx = 2.0f;
    physics.airmov.vy = 3.0f;
    physics.airmov.vz = 4.0f;
    player.pFlags = 1;
    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.mov.vx == 1.0f);
    CHECK(physics.mov.vy == 1.5f);
    CHECK(physics.mov.vz == 2.0f);
    CHECK(physics.airmov.vy == -2.5f);

    jpb_PhysicsBeginObjectFrame(&physics);
    memset(&solid, 0, sizeof(solid));
    physics.solid = &solid;
    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK((physics.flags & 0x00004000u) != 0);
    return 0;
}

static int test_move_player_direct_and_special_paths(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    int old_mapyend = mapyend;
    int old_numsolids = numsolids;
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    int mode;

    CHECK(jpb_PhysicsMovePlayer(NULL) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);
    for (mode = MOVE_FLY; mode <= MOVE_COREDEATH; mode += 2) {
        memset(&physics, 0, sizeof(physics));
        physics.movemode = (MOVE_MODE)mode;
        physics.pos.vx = 10.0f;
        physics.pos.vy = 20.0f;
        physics.pos.vz = 30.0f;
        physics.mov.vx = 1.0f;
        physics.mov.vy = -2.0f;
        physics.mov.vz = 3.0f;
        physics.flags = UINT32_C(0x00011800);

        CHECK(jpb_PhysicsMovePlayer(&physics) ==
              JPB_PHYSICS_PARTIAL_OK);
        CHECK(physics.pos.vx == 11.0f);
        CHECK(physics.pos.vy == 18.0f);
        CHECK(physics.pos.vz == 33.0f);
        CHECK(physics.flags == UINT32_C(0x00010000));
    }

    physics_gInitObjects(0);
    mapyend = 0;
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    actor.objectID = 2;
    model.modelRoot.objectID = 2;
    physics.physicsRoot.objectID = 2;
    animation.animRoot.objectID = 2;
    player.playerRoot.objectID = 2;
    physics.movemode = MOVE_HOVER;
    physics.height = 20;
    physics.radius = 2;
    physics.maxledge = 32;
    physics.pos.vx = 10.0f;
    physics.pos.vy = 20.0f;
    physics.pos.vz = 30.0f;
    physics.mov.vx = 1.0f;
    physics.mov.vy = 2.0f;
    physics.mov.vz = 3.0f;
    physics.flags = UINT32_C(0x00011800);

    CHECK(jpb_PhysicsMovePlayer(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.pos.vx == 11.0f);
    CHECK(physics.pos.vy == 22.0f);
    CHECK(physics.pos.vz == 33.0f);
    CHECK(physics.flags == 0);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_NORMAL;
    physics.pos.vx = 10.0f;
    physics.pos.vy = 20.0f;
    physics.pos.vz = 30.0f;
    physics.mov.vx = 1.0f;
    physics.mov.vy = 2.0f;
    physics.mov.vz = 3.0f;
    physics.flags = UINT32_C(0x00011840);
    player.pFlags = UINT32_C(0x40000008);
    player.playerID = 0;
    player.groundDelay = UINT32_C(0xffffffff);

    CHECK(jpb_PhysicsMovePlayer(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.pos.vx == 11.0f);
    CHECK(physics.pos.vy == 22.0f);
    CHECK(physics.pos.vz == 33.0f);
    CHECK(physics.validairground == 22.0f);
    CHECK(physics.flags == UINT32_C(0x00000040));
    CHECK(player.pFlags == 0);
    CHECK(player.groundDelay == 0);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_NORMAL;
    physics.flags = UINT32_C(0x00000040);
    player.playerID = 0x3e;
    player.pFlags = UINT32_C(0x40000008);

    CHECK(jpb_PhysicsMovePlayer(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(player.pFlags == UINT32_C(0x40000000));

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_NORMAL;
    physics.ledgepoint.vx = 100.0f;
    physics.ledgepoint.vy = 200.0f;
    physics.ledgepoint.vz = 300.0f;
    physics.angle.vy = 0;
    physics.flags = UINT32_C(0x00010000);
    player.pFlags = UINT32_C(0x04000008);

    CHECK(jpb_PhysicsMovePlayer(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.pos.vx == 100.0f);
    CHECK(physics.pos.vy == 200.0f);
    CHECK(physics.pos.vz == 304.0f);
    CHECK(physics.flags == 0);
    CHECK(player.pFlags == UINT32_C(0x40000000));
    leveldata = old_leveldata;
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    return 0;
}

static int test_normal_movement_surface_forces(void)
{
    enum {
        MAP_INDEX = 4,
        MAP_FLAG_SLOPE = 0x00014000,
        MAP_FLAG_MODE_TRANSITION = 0x00040000,
        MAP_FLAG_CONVEYOR_POS_Z = 0x00200000,
        MAP_FLAG_CONVEYOR_NEG_X = 0x01000000
    };
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    physicsObject platform;
    Mnode platform_node;
    _solid platform_solid;
    int32_t map_storage[16];
    int32_t poly;
    int32_t *old_leveldata = leveldata;
    int32_t old_totalframes = totalframes;
    FVECTOR old_gravity = globalgravity;
    float old_frame_rate = fGlobalFrameRate;
    char old_level = LevelSelect;

    memset(map_storage, 0, sizeof(map_storage));
    poly = MAP_INDEX;
    leveldata = map_storage;
    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.currentmapinfo.poly = &poly;
    map_storage[MAP_INDEX] = MAP_FLAG_SLOPE;
    map_storage[MAP_INDEX + 1] = 511;
    physics.constmov.vx = -4.0f;
    physics.constmov.vy = 6.0f;
    physics.constmov.vz = 8.0f;
    player.pFlags = UINT32_C(0x00000008);
    player.runCounter = 7;
    fGlobalFrameRate = 0.5f;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx - 1.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vy - 3.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vz - 4.0f) < 0.00001f);
    CHECK((player.pFlags & UINT32_C(0x10000000)) != 0);
    CHECK((player.pFlags & UINT32_C(0x00000008)) == 0);
    CHECK(player.runCounter == 0);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.currentmapinfo.poly = &poly;
    map_storage[MAP_INDEX] =
        MAP_FLAG_CONVEYOR_POS_Z |
        MAP_FLAG_CONVEYOR_NEG_X;
    physics.constmov.vx = 3.0f;
    physics.constmov.vz = 4.0f;
    fGlobalFrameRate = 0.5f;
    LevelSelect = 1;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx + 9.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vz - 12.5f) < 0.00001f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    memset(&platform, 0, sizeof(platform));
    memset(&platform_node, 0, sizeof(platform_node));
    memset(&platform_solid, 0, sizeof(platform_solid));
    vec_IdentMatrix(&platform_solid.rotmatrix);
    platform_solid.scale.vx = JPB_FIXED_ONE;
    platform_solid.scale.vy = JPB_FIXED_ONE;
    platform_solid.scale.vz = JPB_FIXED_ONE;
    platform_solid.modelnode = &platform_node;
    platform_solid.physics = &platform;
    platform.solid = &platform_solid;
    platform.flags = UINT32_C(0x00004000);
    platform_node.v3RotCenter.vx = 5;
    physics.standee = &platform;
    physics.currentmapinfo.poly = &poly;
    map_storage[MAP_INDEX] =
        MAP_FLAG_CONVEYOR_POS_Z |
        MAP_FLAG_CONVEYOR_NEG_X;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx - 5.0f) < 0.00001f);
    CHECK(physics.mov.vz == 0.0f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.currentmapinfo.poly = &poly;
    map_storage[MAP_INDEX] = 0;
    physics.airmov.vx = 2.0f;
    physics.airmov.vy = 3.0f;
    physics.airmov.vz = 4.0f;
    physics.reversoi = 100;
    player.pFlags = UINT32_C(0x00000001);
    globalgravity.vx = 0.0f;
    globalgravity.vy = 0.0f;
    globalgravity.vz = 0.0f;
    fGlobalFrameRate = 1.0f;
    totalframes = 100 + 0x0f00 + 1;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx + 2.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vy - 3.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vz + 4.0f) < 0.00001f);
    CHECK(physics.reversoi == 0);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.currentmapinfo.poly = &poly;
    physics.airGround = 0.0f;
    physics.validairground = -1000.0f;
    physics.pos.vy = 100.0f;
    map_storage[MAP_INDEX] = MAP_FLAG_MODE_TRANSITION;
    LevelSelect = 10;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);
    CHECK((physics.flags & UINT32_C(0x00004000)) == 0);

    leveldata = old_leveldata;
    totalframes = old_totalframes;
    globalgravity = old_gravity;
    fGlobalFrameRate = old_frame_rate;
    LevelSelect = old_level;
    return 0;
}

static int test_blown_movement_state_machine(void)
{
    enum {
        MAP_INDEX = 4,
        MAP_FLAG_MODE_TRANSITION = 0x00040000
    };
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    Motion motions[5];
    int32_t map_storage[16];
    int32_t poly = MAP_INDEX;
    int32_t *old_leveldata = leveldata;
    int32_t old_fixed_frame_rate = gGlobalFrameRate;
    FVECTOR old_gravity = globalgravity;
    float old_frame_rate = fGlobalFrameRate;
    char old_level = LevelSelect;
    JPBPlayerCallback old_callback =
        jpb_TrajectoryCallbackSlot;

    memset(motions, 0, sizeof(motions));
    memset(map_storage, 0, sizeof(map_storage));
    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    leveldata = map_storage;
    map_storage[MAP_INDEX] = MAP_FLAG_MODE_TRANSITION;
    map_storage[MAP_INDEX + 1] =
        1 | (0x3fe << 10) | (2 << 20);
    physics.currentmapinfo.poly = &poly;
    physics.airGround = 0.0f;
    physics.pos.vy = 100.0f;
    player.paMotions = motions;
    player.pMotionCallBack = test_trajectory_callback;
    LevelSelect = 2;
    gGlobalFrameRate = JPB_FIXED_ONE;
    fGlobalFrameRate = 1.0f;
    globalgravity.vx = 0.0f;
    globalgravity.vy = -11.0f;
    globalgravity.vz = 0.0f;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.movemode == MOVE_BLOWN);
    CHECK(physics.userdata[0] == 0x200);
    CHECK(physics.uservector.vx == 40);
    CHECK(physics.uservector.vy == -80);
    CHECK(physics.uservector.vz == 80);
    CHECK(fabsf(
              physics.airmov.vx -
              40.0f / 4096.0f) <
          0.000001f);
    CHECK(fabsf(
              physics.airmov.vy -
              -80.0f / 4096.0f) <
          0.000001f);
    CHECK(fabsf(
              physics.airmov.vz -
              80.0f / 4096.0f) <
          0.000001f);
    CHECK(physics.mov.vx == physics.airmov.vx);
    CHECK(physics.mov.vy == physics.airmov.vy);
    CHECK(physics.mov.vz == physics.airmov.vz);
    CHECK((player.pFlags & UINT32_C(0x00000001)) != 0);
    CHECK(player.pMotionCallBack == NULL);

    jpb_PhysicsBeginObjectFrame(&physics);
    physics.userdata[0] = 0x2800;
    physics.airmov.vx = 1.0f;
    physics.airmov.vy = 2.0f;
    physics.airmov.vz = 3.0f;
    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.movemode == MOVE_BLOWN);
    CHECK(physics.userdata[0] == 0x2a00);
    CHECK(physics.airmov.vx == 1.0f);
    CHECK(physics.airmov.vy == -9.0f);
    CHECK(physics.airmov.vz == 3.0f);
    CHECK(physics.mov.vx == 1.0f);
    CHECK(physics.mov.vy == -9.0f);
    CHECK(physics.mov.vz == 3.0f);

    jpb_PhysicsBeginObjectFrame(&physics);
    physics.userdata[0] = 0xa001;
    physics.airTime = 123;
    physics.realAirTime = 456;
    physics.airmov.vx = 2.0f;
    physics.airmov.vy = 3.0f;
    physics.airmov.vz = 4.0f;
    globalgravity.vy = 0.0f;
    jpb_TrajectoryCallbackSlot =
        test_trajectory_callback;
    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.movemode == MOVE_NORMAL);
    CHECK(physics.airTime == 0);
    CHECK(physics.realAirTime == 0);
    CHECK(physics.mov.vx == 2.0f);
    CHECK(physics.mov.vy == 3.0f);
    CHECK(physics.mov.vz == 4.0f);
    CHECK(player.pMotionCallBack ==
          test_trajectory_callback);

    leveldata = old_leveldata;
    gGlobalFrameRate = old_fixed_frame_rate;
    globalgravity = old_gravity;
    fGlobalFrameRate = old_frame_rate;
    LevelSelect = old_level;
    jpb_TrajectoryCallbackSlot = old_callback;
    return 0;
}

static int test_hover_movement_state_machine(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    float old_frame_rate = fGlobalFrameRate;
    uint32_t old_cube_flags = jpb_CubeRuntimeFlags;

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000900;
    physics.airGround = 0.0f;
    physics.pos.vy = 89.0f;
    physics.constmov.vx = 1.0f;
    physics.constmov.vy = 2.0f;
    physics.constmov.vz = 4.0f;
    physics.airmov.vx = 2.0f;
    physics.airmov.vy = 30.0f;
    physics.airmov.vz = 3.0f;
    fGlobalFrameRate = 0.5f;
    jpb_CubeRuntimeFlags = 0;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.airmov.vy == 32.0f);
    CHECK(fabsf(physics.mov.vx - 1.5f) < 0.00001f);
    CHECK(fabsf(physics.mov.vy - 17.0f) < 0.00001f);
    CHECK(fabsf(physics.mov.vz - 3.5f) < 0.00001f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000900;
    physics.airGround = 0.0f;
    physics.pos.vy = 90.0f;
    physics.airmov.vy = 20.0f;
    fGlobalFrameRate = 1.0f;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.airmov.vy == 4.0f);
    CHECK(physics.mov.vy == 4.0f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000900;
    physics.airGround = 0.0f;
    physics.pos.vy = 120.0f;
    physics.airmov.vy = -16.0f;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.airmov.vy == 0.0f);
    CHECK(physics.mov.vy == 0.0f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000900;
    physics.airGround = 0.0f;
    physics.pos.vy = 121.0f;
    physics.airmov.vy = -31.0f;
    jpb_CubeRuntimeFlags = 0;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.airmov.vy == -32.0f);
    CHECK(physics.mov.vy == -32.0f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000900;
    physics.airGround = 0.0f;
    physics.pos.vy = 121.0f;
    physics.airmov.vy = -63.0f;
    jpb_CubeRuntimeFlags = UINT32_C(0x00000008);

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.airmov.vy == -64.0f);
    CHECK(physics.mov.vy == -64.0f);

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_HOVER;
    physics.angle.vy = 0x00000a00;
    physics.airGround = 0.0f;
    physics.pos.vy = 100.0f;
    physics.userdata[1] = 1024;
    model.v3Scale.vx = JPB_FIXED_ONE / 2;
    jpb_CubeRuntimeFlags = 0;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(physics.mov.vx - 0.5f) < 0.00001f);
    CHECK(physics.mov.vy == 0.0f);
    CHECK(fabsf(physics.mov.vz) < 0.00001f);

    fGlobalFrameRate = old_frame_rate;
    jpb_CubeRuntimeFlags = old_cube_flags;
    return 0;
}

static int test_flying_movement_state_machine(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    wsl_ENEMY enemy;
    float old_frame_rate = fGlobalFrameRate;
    int mode;

    for (mode = MOVE_HOVER3D; mode <= MOVE_FLY; ++mode) {
        connect_complete_actor(
            &actor,
            &scene,
            &model,
            &physics,
            &animation,
            &player);
        memset(&enemy, 0, sizeof(enemy));
        physics.movemode = (MOVE_MODE)mode;
        physics.pos.vx = 10.0f;
        physics.pos.vy = 20.0f;
        physics.pos.vz = 30.0f;
        physics.constmov.vz = 20.0f;
        enemy.destination.vx = 10;
        enemy.destination.vy = 20;
        enemy.destination.vz = 40;
        player.pEnemy = &enemy;
        model.v3Scale.vx = JPB_FIXED_ONE / 2;
        fGlobalFrameRate = 0.5f;

        CHECK(jpb_PhysicsCalcMovement(&physics) ==
              JPB_PHYSICS_PARTIAL_OK);
        CHECK(physics.mov.vx == 0.0f);
        CHECK(physics.mov.vy == 0.0f);
        CHECK(fabsf(physics.mov.vz - 5.0f) < 0.00001f);
    }

    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    physics.movemode = MOVE_FLY;
    physics.constmov.vx = 2.0f;
    physics.constmov.vy = 4.0f;
    physics.constmov.vz = 6.0f;
    player.pEnemy = NULL;
    fGlobalFrameRate = 0.5f;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.mov.vx == 1.0f);
    CHECK(physics.mov.vy == 2.0f);
    CHECK(physics.mov.vz == 3.0f);

    fGlobalFrameRate = old_frame_rate;
    return 0;
}

static int test_coredeath_movement_state_machine(void)
{
    enum {
        MAP_INDEX = 4,
        MAP_FLAG_MODE_TRANSITION = 0x00040000
    };
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    playerObject player;
    Motion motions[5];
    TestSoundStopCapture stop_capture;
    CharacterData old_character = GameStruct.aCharacterData[0];
    uint32_t old_game_state = GameStruct.GameState;
    playerObject *old_after_life = afterLife;
    int32_t map_storage[16];
    int32_t poly = MAP_INDEX;
    int32_t *old_leveldata = leveldata;
    float old_frame_rate = fGlobalFrameRate;
    char old_level = LevelSelect;

    memset(motions, 0, sizeof(motions));
    memset(&stop_capture, 0, sizeof(stop_capture));
    memset(map_storage, 0, sizeof(map_storage));
    connect_complete_actor(
        &actor, &scene, &model, &physics, &animation, &player);
    leveldata = map_storage;
    map_storage[MAP_INDEX] = MAP_FLAG_MODE_TRANSITION;
    physics.currentmapinfo.poly = &poly;
    physics.airGround = 0.0f;
    physics.pos.vx = 10.75f;
    physics.pos.vy = 100.0f;
    physics.pos.vz = 20.75f;
    player.paMotions = motions;
    player.playernum = 0;
    animation.loopHandle[0] = 11;
    animation.loopHandle[1] = 22;
    LevelSelect = 10;
    fGlobalFrameRate = 1.0f;
    jpb_SoundSetStopHook(
        test_sound_stop_hook, &stop_capture);

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(physics.movemode == MOVE_COREDEATH);
    CHECK(physics.uservector.vx == 10);
    CHECK(physics.uservector.vz == 20);
    CHECK(physics.userdata[0] == 6);
    CHECK(fabsf(
              physics.mov.vx + 0.70710677f) <
          0.00001f);
    CHECK(physics.mov.vy == 6.0f);
    CHECK(fabsf(
              physics.mov.vz + 0.70710677f) <
          0.00001f);
    CHECK((player.pFlags & UINT32_C(0x00000001)) != 0);

    jpb_PhysicsBeginObjectFrame(&physics);
    physics.userdata[0] = 200;
    player.whohitme = &player;
    GameStruct.aCharacterData[0].Energy = 100;
    GameStruct.aCharacterData[0].MaxEnergy = 100;
    GameStruct.GameState = 0;
    afterLife = NULL;

    CHECK(jpb_PhysicsCalcMovement(&physics) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(stop_capture.count == 2);
    CHECK(stop_capture.handles[0] == 11);
    CHECK(stop_capture.handles[1] == 22);
    CHECK(physics.movemode == MOVE_NORMAL);
    CHECK(physics.userdata[0] == 0);
    CHECK(physics.mov.vy == 96.0f);
    CHECK((player.pFlags & UINT32_C(0x00000200)) != 0);
    CHECK(GameStruct.aCharacterData[0].Energy == 0);
    CHECK((GameStruct.GameState & UINT32_C(0x00000020)) != 0);
    CHECK((actor.flags & UINT32_C(0x00000020)) != 0);
    CHECK(afterLife == &player);
    CHECK(player.whohitme == NULL);

    jpb_SoundSetStopHook(NULL, NULL);
    GameStruct.aCharacterData[0] = old_character;
    GameStruct.GameState = old_game_state;
    afterLife = old_after_life;
    leveldata = old_leveldata;
    fGlobalFrameRate = old_frame_rate;
    LevelSelect = old_level;
    return 0;
}

static int test_solid_relative_world_transforms(void)
{
    _solid solid;
    Mnode node;
    physicsObject platform;
    physicsObject rider;
    FVECTOR world;
    FVECTOR relative;
    FVECTOR restored;

    memset(&solid, 0, sizeof(solid));
    memset(&node, 0, sizeof(node));
    memset(&platform, 0, sizeof(platform));
    memset(&rider, 0, sizeof(rider));
    vec_IdentMatrix(&solid.rotmatrix);
    solid.scale.vx = JPB_FIXED_ONE;
    solid.scale.vy = JPB_FIXED_ONE;
    solid.scale.vz = JPB_FIXED_ONE;
    solid.modelnode = &node;
    solid.physics = &platform;
    node.v3RotCenter.vx = 100;
    node.v3RotCenter.vy = -50;
    node.v3RotCenter.vz = 25;
    world.vx = 110.0f;
    world.vy = -30.0f;
    world.vz = 55.0f;

    CalcRelativePosFromWorld(&solid, &world, &relative);
    CHECK(relative.vx == 10.0f);
    CHECK(relative.vy == 20.0f);
    CHECK(relative.vz == 30.0f);
    CalcWorldPosFromRelative(&solid, &relative, &restored);
    CHECK(restored.vx == world.vx);
    CHECK(restored.vy == world.vy);
    CHECK(restored.vz == world.vz);

    solid.rotmatrix.m[0][0] = 0.0f;
    solid.rotmatrix.m[0][1] = 0.0f;
    solid.rotmatrix.m[0][2] = 1.0f;
    solid.rotmatrix.m[1][0] = 0.0f;
    solid.rotmatrix.m[1][1] = 1.0f;
    solid.rotmatrix.m[1][2] = 0.0f;
    solid.rotmatrix.m[2][0] = -1.0f;
    solid.rotmatrix.m[2][1] = 0.0f;
    solid.rotmatrix.m[2][2] = 0.0f;
    CalcRelativePosFromWorld(&solid, &world, &relative);
    CHECK(relative.vx == -30.0f);
    CHECK(relative.vy == 20.0f);
    CHECK(relative.vz == 10.0f);
    CalcWorldPosFromRelative(&solid, &relative, &restored);
    CHECK(restored.vx == world.vx);
    CHECK(restored.vy == world.vy);
    CHECK(restored.vz == world.vz);

    vec_IdentMatrix(&solid.rotmatrix);
    platform.angle.vx = 100;
    platform.angle.vy = 200;
    platform.angle.vz = 300;
    rider.angle.vx = 140;
    rider.angle.vy = 150;
    rider.angle.vz = 500;
    rider.flags = 0x00011800u;
    CalcSolidRelativePos(&solid, &rider, &world);
    CHECK(rider.localpos.vx == 10.0f);
    CHECK(rider.localpos.vy == 20.0f);
    CHECK(rider.localpos.vz == 30.0f);
    CHECK(rider.localfacing.vx == 40.0f);
    CHECK(rider.localfacing.vy == -50.0f);
    CHECK(rider.localfacing.vz == 200.0f);
    CHECK(rider.flags == 0x00010000u);

    rider.localpos.vx = 91.0f;
    rider.flags = 0;
    CalcSolidRelativePos(&solid, &rider, &world);
    CHECK(rider.localpos.vx == 91.0f);
    return 0;
}

static int test_processed_standee_movement(void)
{
    objectRoot actor;
    sceneObject scene;
    modelObject model;
    physicsObject rider;
    physicsObject platform;
    animObject animation;
    playerObject player;
    objectRoot platform_actor;
    sceneObject platform_scene;
    modelObject platform_model;
    animObject platform_animation;
    playerObject platform_player;
    Mnode platform_node;
    _solid platform_solid;

    connect_complete_actor(
        &actor, &scene, &model, &rider, &animation, &player);
    connect_complete_actor(
        &platform_actor,
        &platform_scene,
        &platform_model,
        &platform,
        &platform_animation,
        &platform_player);
    memset(&platform_node, 0, sizeof(platform_node));
    memset(&platform_solid, 0, sizeof(platform_solid));
    vec_IdentMatrix(&platform_solid.rotmatrix);
    platform_solid.scale.vx = JPB_FIXED_ONE;
    platform_solid.scale.vy = JPB_FIXED_ONE;
    platform_solid.scale.vz = JPB_FIXED_ONE;
    platform_solid.modelnode = &platform_node;
    platform_solid.physics = &platform;
    platform.solid = &platform_solid;
    platform.flags = 0x00004000u;
    platform.angle.vy = 100;
    platform_node.v3RotCenter.vx = 5;

    fGlobalFrameRate = 1.0f;
    rider.standee = &platform;
    rider.localpos.vx = 10.0f;
    rider.newlocalpos.vx = 10.0f;
    rider.localfacing.vy = 20.0f;
    CHECK(jpb_PhysicsCalcMovementNormal(&rider) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(rider.angle.vy == 120);
    CHECK(rider.newlocalpos.vx == 15.0f);
    CHECK(rider.mov.vx == 5.0f);
    CHECK(rider.mov.vy == 0.0f);
    CHECK(rider.mov.vz == 0.0f);

    jpb_PhysicsBeginObjectFrame(&rider);
    jpb_PhysicsBeginObjectFrame(&platform);
    platform.angle.vy = 0;
    platform.constmov.vz = 12.0f;
    platform.accel.vz = 2.0f;
    CHECK(jpb_PhysicsCalcMovementNormal(&rider) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK((rider.flags & 0x00004000u) != 0);
    CHECK((platform.flags & 0x00004000u) != 0);
    CHECK(platform.constmov.vz == 10.0f);
    CHECK(platform.currentmov.vz == 10.0f);
    CHECK(platform.mov.vz == 10.0f);
    return 0;
}

static int test_recursive_standee_cycle_scheduling(void)
{
    objectRoot actor_a;
    objectRoot actor_b;
    sceneObject scene_a;
    sceneObject scene_b;
    modelObject model_a;
    modelObject model_b;
    physicsObject physics_a;
    physicsObject physics_b;
    animObject animation_a;
    animObject animation_b;
    playerObject player_a;
    playerObject player_b;
    Mnode node_a;
    Mnode node_b;
    _solid solid_a;
    _solid solid_b;

    connect_complete_actor(
        &actor_a,
        &scene_a,
        &model_a,
        &physics_a,
        &animation_a,
        &player_a);
    connect_complete_actor(
        &actor_b,
        &scene_b,
        &model_b,
        &physics_b,
        &animation_b,
        &player_b);
    memset(&node_a, 0, sizeof(node_a));
    memset(&node_b, 0, sizeof(node_b));
    memset(&solid_a, 0, sizeof(solid_a));
    memset(&solid_b, 0, sizeof(solid_b));
    vec_IdentMatrix(&solid_a.rotmatrix);
    vec_IdentMatrix(&solid_b.rotmatrix);
    solid_a.scale.vx = JPB_FIXED_ONE;
    solid_a.scale.vy = JPB_FIXED_ONE;
    solid_a.scale.vz = JPB_FIXED_ONE;
    solid_b.scale.vx = JPB_FIXED_ONE;
    solid_b.scale.vy = JPB_FIXED_ONE;
    solid_b.scale.vz = JPB_FIXED_ONE;
    solid_a.modelnode = &node_a;
    solid_a.physics = &physics_a;
    solid_b.modelnode = &node_b;
    solid_b.physics = &physics_b;
    physics_a.solid = &solid_a;
    physics_a.standee = &physics_b;
    physics_b.solid = &solid_b;
    physics_b.standee = &physics_a;
    physics_a.constmov.vz = 10.0f;
    physics_a.accel.vz = 1.0f;
    physics_b.constmov.vz = 20.0f;
    physics_b.accel.vz = 2.0f;
    fGlobalFrameRate = 1.0f;

    CHECK(jpb_PhysicsCalcMovementNormal(&physics_a) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK((physics_a.flags & UINT32_C(0x00004000)) != 0);
    CHECK((physics_b.flags & UINT32_C(0x00004000)) != 0);
    CHECK(physics_a.constmov.vz == 9.0f);
    CHECK(physics_a.mov.vz == 9.0f);
    CHECK(physics_b.constmov.vz == 18.0f);
    CHECK(physics_b.mov.vz == 18.0f);

    CHECK(jpb_PhysicsCalcMovementNormal(&physics_a) ==
          JPB_PHYSICS_PARTIAL_ALREADY_PROCESSED);
    CHECK(physics_a.constmov.vz == 9.0f);
    CHECK(physics_b.constmov.vz == 18.0f);

    jpb_PhysicsBeginObjectFrame(&physics_a);
    jpb_PhysicsBeginObjectFrame(&physics_b);
    CHECK(jpb_PhysicsCalcMovementNormal(&physics_b) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK((physics_a.flags & UINT32_C(0x00004000)) != 0);
    CHECK((physics_b.flags & UINT32_C(0x00004000)) != 0);
    CHECK(physics_a.constmov.vz == 8.0f);
    CHECK(physics_b.constmov.vz == 16.0f);
    return 0;
}

static int test_contact_geometry_helpers(void)
{
    WorldData world_data;
    WorldData *old_world = gpWorld;
    FVECTOR v;
    FVECTOR normalized;
    FVECTOR4 frustplane[5];
    FVECTOR4 box_planes[4];
    FVECTOR4 collide[5];
    MATRIX matrix;
    VECTOR campos;
    float length;

    v.vx = 3.0f;
    v.vy = 4.0f;
    v.vz = 0.0f;
    length = VectorNormalize(&v);
    CHECK(fabsf(length - 5.0f) < 0.00001f);
    CHECK(fabsf(v.vx - 0.6f) < 0.00001f);
    CHECK(fabsf(v.vy - 0.8f) < 0.00001f);
    CHECK(v.vz == 0.0f);
    v.vx = 0.0f;
    v.vy = 0.0f;
    v.vz = -2.0f;
    CHECK(VectorNormalize2(&v, &normalized) == 2.0f);
    CHECK(normalized.vx == 0.0f);
    CHECK(normalized.vy == 0.0f);
    CHECK(normalized.vz == -1.0f);
    normalized.vx = 99.0f;
    normalized.vy = 99.0f;
    normalized.vz = 99.0f;
    CHECK(VectorNormalize3(0.0f, 0.0f, 0.0f, &normalized) == 0.0f);
    CHECK(normalized.vx == 0.0f);
    CHECK(normalized.vy == 0.0f);
    CHECK(normalized.vz == 0.0f);

    memset(&world_data, 0, sizeof(world_data));
    memset(frustplane, 0, sizeof(frustplane));
    world_data.start.vx = 10;
    world_data.start.vz = 30;
    gpWorld = &world_data;
    frustplane[0].vx = 3.0f;
    frustplane[0].vy = 4.0f;
    frustplane[0].vw = 7.0f;
    frustplane[1].vy = 1.0f;
    frustplane[2].vz = 4.0f;
    frustplane[2].vw = 8.0f;
    frustplane[3].vx = 3.0f;
    frustplane[3].vy = -2.0f;
    frustplane[3].vz = 4.0f;
    frustplane[3].vw = 5.0f;
    CHECK(CalcNewBox(40, frustplane, box_planes) == 16);
    CHECK(box_planes[0].vx == 1.0f);
    CHECK(box_planes[0].vy == 0.0f);
    CHECK(box_planes[0].vz == 0.0f);
    CHECK(box_planes[0].vw == -51.0f);
    CHECK(box_planes[1].vw == 65536.0f);
    CHECK(box_planes[2].vz == 1.0f);
    CHECK(box_planes[2].vw == 2.0f);
    CHECK(fabsf(box_planes[3].vx - 0.6f) < 0.00001f);
    CHECK(fabsf(box_planes[3].vz - 0.8f) < 0.00001f);
    CHECK(box_planes[3].vw == 17.0f);

    vec_IdentMatrix(&matrix);
    memset(&campos, 0, sizeof(campos));
    campos.vx = 10;
    campos.vy = 20;
    campos.vz = 30;
    buildplane(&matrix, &campos, &collide[0], 1.0f, 0.0f, 0.0f);
    CHECK(collide[0].vx == -1.0f);
    CHECK(collide[0].vy == 0.0f);
    CHECK(collide[0].vz == 0.0f);
    CHECK(collide[0].vw == -10.0f);
    buildfrustrum(&matrix, collide, &campos, 100.0f, 320.0f, 240.0f);
    CHECK(fabsf(
              sqrtf(
                  collide[0].vx * collide[0].vx +
                  collide[0].vy * collide[0].vy +
                  collide[0].vz * collide[0].vz) -
              1.0f) <
          0.00001f);
    CHECK(collide[0].vx < 0.0f);
    CHECK(collide[1].vx > 0.0f);
    CHECK(collide[2].vz < 0.0f);
    CHECK(collide[3].vz > 0.0f);
    CHECK(collide[4].vx == 0.0f);
    CHECK(collide[4].vy == -1.0f);
    CHECK(collide[4].vz == 0.0f);
    CHECK(collide[4].vw == -20.0f);
    gpWorld = old_world;
    return 0;
}

static int test_plane_contact_helper(void)
{
    FVECTOR4 plane = {0.0f, 1.0f, 0.0f, 0.0f};

    memset(&mvp, 0, sizeof(mvp));
    memset(&bestinfo, 0, sizeof(bestinfo));
    mvp.movement.vy = -0.5f;
    mvp.startpos.vy = 4.0f;
    mvp.distance = 10.0f;
    mvp.info.flags = 0x1234;
    mvp.info.kisspoint.vx = 11.0f;
    mvp.info.edge = 7;
    bestinfo.dist = 100.0f;

    CHECK(jpb_PhysicsPlaneCheck(1, &plane) == 1);
    CHECK(mvp.radius == 1.0f);
    CHECK(mvp.info.type == 1);
    CHECK(mvp.info.dist == 6.0f);
    CHECK(mvp.info.n.vx == 0.0f);
    CHECK(mvp.info.n.vy == -1.0f);
    CHECK(mvp.info.n.vz == 0.0f);
    CHECK(mvp.info.facenormal.vx == 0.0f);
    CHECK(mvp.info.facenormal.vy == 1.0f);
    CHECK(mvp.info.facenormal.vz == 0.0f);
    CHECK(bestinfo.type == 9);
    CHECK(bestinfo.flags == 0x1234);
    CHECK(bestinfo.dist == 6.0f);
    CHECK(bestinfo.kisspoint.vx == 11.0f);
    CHECK(bestinfo.edge == 7);
    CHECK(bestinfo.washack == 0);

    bestinfo.dist = 5.0f;
    CHECK(jpb_PhysicsPlaneCheck(1, &plane) == 0);
    CHECK(bestinfo.dist == 5.0f);

    mvp.startpos.vy = 0.0f;
    bestinfo.dist = 5.0f;
    CHECK(jpb_PhysicsPlaneCheck(1, &plane) == 1);
    CHECK(bestinfo.dist == 0.0f);

    mvp.movement.vy = 0.5f;
    bestinfo.dist = 5.0f;
    CHECK(jpb_PhysicsPlaneCheck(1, &plane) == 0);
    return 0;
}

static void set_square_contact_packet(float x, float y, float z)
{
    memset(&mvp, 0, sizeof(mvp));
    memset(&cvars, 0, sizeof(cvars));
    mvp.points[0].vx = -10.0f;
    mvp.points[0].vz = -10.0f;
    mvp.points[1].vx = 10.0f;
    mvp.points[1].vz = -10.0f;
    mvp.points[2].vx = 10.0f;
    mvp.points[2].vz = 10.0f;
    mvp.points[3].vx = -10.0f;
    mvp.points[3].vz = 10.0f;
    mvp.facenormal.vy = 1.0f;
    mvp.movement.vy = -1.0f;
    mvp.startpos.vx = x;
    mvp.startpos.vy = y;
    mvp.startpos.vz = z;
    mvp.vmin.vx = -100.0f;
    mvp.vmin.vy = -100.0f;
    mvp.vmin.vz = -100.0f;
    mvp.vmax.vx = 100.0f;
    mvp.vmax.vy = 100.0f;
    mvp.vmax.vz = 100.0f;
    mvp.radius = 1.0f;
    mvp.distance = 10.0f;
    mvp.numsides = 4;
}

static int test_sphere_polygon_contact_kernel(void)
{
    set_square_contact_packet(0.0f, 5.0f, 0.0f);
    CHECK(jpb_PhysicsSphereAndPoly() == 1);
    CHECK(mvp.info.type == 1);
    CHECK(fabsf(mvp.info.dist - 4.0f) < 0.00001f);
    CHECK(mvp.info.kisspoint.vx == 0.0f);
    CHECK(mvp.info.kisspoint.vy == 0.0f);
    CHECK(mvp.info.kisspoint.vz == 0.0f);

    set_square_contact_packet(0.0f, 0.5f, 0.0f);
    CHECK(jpb_PhysicsSphereAndPoly() == 1);
    CHECK(mvp.info.type == 5);
    CHECK(mvp.info.dist == 0.5f);
    CHECK(mvp.info.n.vy == 1.0f);

    set_square_contact_packet(10.5f, 5.0f, 0.0f);
    CHECK(jpb_PhysicsSphereAndPoly() == 1);
    CHECK(mvp.info.type == 2);
    CHECK(mvp.info.edge == 1);
    CHECK(mvp.info.kisspoint.vx == 10.0f);
    CHECK(mvp.info.kisspoint.vy == 0.0f);

    set_square_contact_packet(10.5f, 5.0f, 10.5f);
    CHECK(jpb_PhysicsSphereAndPoly() == 1);
    CHECK(mvp.info.type == 3);
    CHECK(mvp.info.kisspoint.vx == 10.0f);
    CHECK(mvp.info.kisspoint.vy == 0.0f);
    CHECK(mvp.info.kisspoint.vz == 10.0f);

    set_square_contact_packet(0.0f, 5.0f, 0.0f);
    mvp.movement.vy = 1.0f;
    CHECK(jpb_PhysicsSphereAndPoly() == 0);

    set_square_contact_packet(0.0f, 5.0f, 0.0f);
    mvp.vmin.vx = 20.0f;
    mvp.vmax.vx = 30.0f;
    CHECK(jpb_PhysicsSphereAndPoly() == 0);

    set_square_contact_packet(0.0f, 5.0f, 0.0f);
    memset(&bestinfo, 0, sizeof(bestinfo));
    bestinfo.dist = 11.0f;
    CHECK(jpb_PhysicsPolyCollideCheck() == 1);
    CHECK(bestinfo.type == 1);
    CHECK(fabsf(bestinfo.dist - 4.0f) < 0.00001f);

    set_square_contact_packet(0.0f, 5.0f, 0.0f);
    bestinfo.type = 4;
    bestinfo.dist = 0.0f;
    CHECK(jpb_PhysicsPolyCollideCheck() == 0);
    CHECK(bestinfo.type == 4);
    return 0;
}

static int test_general_solid_collision(void)
{
    _solid solid;
    geomData geometry;
    _svector vertices[4];
    _svector normals[1];
    int16_t indices[4] = {0, 1, 2, 3};
    FVECTOR movement = {0.0f, -1.0f, 0.0f};
    FVECTOR from = {0.0f, 5.0f, 0.0f};
    int index_id;

    memset(&solid, 0, sizeof(solid));
    memset(&geometry, 0, sizeof(geometry));
    memset(vertices, 0, sizeof(vertices));
    memset(normals, 0, sizeof(normals));
    pointerRegistry_Reset();

    vertices[0].vx = -10;
    vertices[0].vz = -10;
    vertices[1].vx = 10;
    vertices[1].vz = -10;
    vertices[2].vx = 10;
    vertices[2].vz = 10;
    vertices[3].vx = -10;
    vertices[3].vz = 10;
    normals[0].vy = 4096;
    index_id = addPtr(indices, 4);
    CHECK(index_id >= 0);

    geometry.numFaces = 1;
    geometry.pIndex = index_id;
    solid.geometry = &geometry;
    solid.coords = vertices;
    solid.normals = normals;

    memset(&mvp, 0, sizeof(mvp));
    memset(&bestinfo, 0, sizeof(bestinfo));
    mvp.startpos = from;
    mvp.vmin.vx = -100.0f;
    mvp.vmin.vy = -100.0f;
    mvp.vmin.vz = -100.0f;
    mvp.vmax.vx = 100.0f;
    mvp.vmax.vy = 100.0f;
    mvp.vmax.vz = 100.0f;
    mvp.radius = 1.0f;
    bestinfo.dist = 11.0f;

    CHECK(jpb_PhysicsGeneralCollide(
              &solid, &movement, &from, 10.0f, 1.0f) ==
          1);
    CHECK(mvp.numsides == 4);
    CHECK(mvp.info.flags == 6);
    CHECK(mvp.facenormal.vx == 0.0f);
    CHECK(mvp.facenormal.vy == 1.0f);
    CHECK(mvp.facenormal.vz == 0.0f);
    CHECK(bestinfo.type == 1);
    CHECK(bestinfo.flags == 6);
    CHECK(fabsf(bestinfo.dist - 4.0f) < 0.00001f);
    CHECK(bestinfo.kisspoint.vy == 0.0f);

    movement.vy = 1.0f;
    bestinfo.type = 0;
    bestinfo.dist = 11.0f;
    CHECK(jpb_PhysicsGeneralCollide(
              &solid, &movement, &from, 10.0f, 1.0f) ==
          0);
    pointerRegistry_Reset();
    return 0;
}

static int test_libpart_float_decode(void)
{
    int32_t storage[80];
    int32_t *mapbase = storage + 4;
    int32_t cube = 10;
    int32_t numverts = 0;
    FVECTOR output[16];
    FVECTOR cubeorg = {0.0f, 0.0f, 0.0f};
    uint16_t *packed_vertex;

    memset(storage, 0, sizeof(storage));
    memset(output, 0, sizeof(output));
    mapbase[10] = (1 << 16) | 50;
    mapbase[11] = 1 << 15;
    packed_vertex = (uint16_t *)((uint8_t *)mapbase + 100);
    *packed_vertex = (uint16_t)(2 | (1 << 5) | (1 << 10));

    CHECK(jon_getlibpartfloat(
              output, &cube, &cubeorg, mapbase, &numverts) ==
          &mapbase[10]);
    CHECK(numverts == 9);
    CHECK(output[0].vx == 256.0f);
    CHECK(output[0].vy == 0.0f);
    CHECK(output[0].vz == 0.0f);
    CHECK(output[3].vx == 0.0f);
    CHECK(output[3].vz == 256.0f);
    CHECK(output[4].vy == 512.0f);
    CHECK(output[8].vx == 224.0f);
    CHECK(output[8].vy == 16.0f);
    CHECK(output[8].vz == 16.0f);
    return 0;
}

static int test_newclosest_poly_thin_map(void)
{
    enum {
        MAP_WORDS = 33000,
        NORMAL_INDEX = 100,
        LIB_INDEX = 200,
        CUBE_INDEX = 300,
        CELL_INDEX = 127 * 256 + 128
    };
    static int32_t storage[MAP_WORDS + 4];
    int32_t *old_leveldata = leveldata;
    int32_t old_mapyend = mapyend;
    int32_t old_numsolids = numsolids;
    int32_t *cubehit = (int32_t *)(uintptr_t)1;
    int32_t *entryhit = (int32_t *)(uintptr_t)1;
    int32_t *polyhit = (int32_t *)(uintptr_t)1;
    FVECTOR from = {128.0f, 10.0f, 64.0f};
    FVECTOR to = {128.0f, -10.0f, 64.0f};
    FVECTOR move = {0.0f, -20.0f, 0.0f};
    FVECTOR movenormal = {0.0f, -1.0f, 0.0f};

    memset(storage, 0, sizeof(storage));
    leveldata = storage + 4;
    mapyend = 256;
    physics_gInitObjects(0);
    numsolids = 0;

    leveldata[NORMAL_INDEX] = 0;
    leveldata[NORMAL_INDEX + 1] = 511 << 10;
    leveldata[LIB_INDEX] = 0;
    leveldata[LIB_INDEX + 1] = 0;
    leveldata[LIB_INDEX + 2] =
        (int32_t)(UINT32_C(0x40000000) | NORMAL_INDEX);
    leveldata[LIB_INDEX + 3] =
        (1 << 20) | (2 << 5) | (1 << 10);
    leveldata[CUBE_INDEX] =
        (int32_t)(
            UINT32_C(0x40000000) |
            (UINT32_C(2) << 26) |
            UINT32_C(0x400));
    leveldata[CUBE_INDEX + 1] = 0;
    leveldata[CUBE_INDEX + 2] = LIB_INDEX;
    leveldata[CELL_INDEX] =
        (int32_t)(UINT32_C(0x80000000) | CUBE_INDEX);

    CHECK(newclosestPoly(
              &from,
              &to,
              &move,
              20.0f,
              1.0f,
              &movenormal,
              2,
              &cubehit,
              &entryhit,
              &polyhit) ==
          1);
    CHECK(cubehit == &leveldata[CUBE_INDEX]);
    CHECK(entryhit == &leveldata[CUBE_INDEX + 2]);
    CHECK(polyhit == &leveldata[LIB_INDEX + 2]);
    CHECK(bestinfo.type == 1);
    CHECK(bestinfo.flags == 0);
    CHECK(bestinfo.dist > 8.99f);
    CHECK(bestinfo.dist < 9.0f);
    CHECK(bestinfo.facenormal.vy > 0.99f);
    CHECK(fabsf(bestinfo.kisspoint.vy) < 0.01f);

    from.vy = -10.0f;
    to.vy = 10.0f;
    move.vy = 20.0f;
    movenormal.vy = 1.0f;
    cubehit = (int32_t *)(uintptr_t)1;
    entryhit = (int32_t *)(uintptr_t)1;
    polyhit = (int32_t *)(uintptr_t)1;
    CHECK(newclosestPoly(
              &from,
              &to,
              &move,
              20.0f,
              1.0f,
              &movenormal,
              2,
              &cubehit,
              &entryhit,
              &polyhit) ==
          0);
    CHECK(cubehit == (int32_t *)(uintptr_t)1);
    CHECK(entryhit == (int32_t *)(uintptr_t)1);
    CHECK(polyhit == (int32_t *)(uintptr_t)1);

    leveldata = old_leveldata;
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    return 0;
}

static int test_newclosest_poly_dynamic_solid(void)
{
    static int32_t storage[33004];
    int32_t *old_leveldata = leveldata;
    int32_t old_mapyend = mapyend;
    int32_t old_numsolids = numsolids;
    sceneObject scene;
    Mnode node;
    _solid solid;
    geomData geometry;
    _svector vertices[4];
    _svector normals[1];
    int16_t indices[4] = {0, 1, 2, 3};
    int32_t *cubehit = (int32_t *)(uintptr_t)1;
    int32_t *entryhit = (int32_t *)(uintptr_t)1;
    int32_t *polyhit = (int32_t *)(uintptr_t)1;
    FVECTOR from = {0.0f, 5.0f, 0.0f};
    FVECTOR to = {0.0f, -5.0f, 0.0f};
    FVECTOR move = {0.0f, -1.0f, 0.0f};
    FVECTOR movenormal = {0.0f, -1.0f, 0.0f};
    int index_id;

    memset(storage, 0, sizeof(storage));
    memset(&scene, 0, sizeof(scene));
    memset(&node, 0, sizeof(node));
    memset(&solid, 0, sizeof(solid));
    memset(&geometry, 0, sizeof(geometry));
    memset(vertices, 0, sizeof(vertices));
    memset(normals, 0, sizeof(normals));
    leveldata = storage + 4;
    mapyend = 256;
    physics_gInitObjects(0);
    pointerRegistry_Reset();

    vertices[0].vx = -10;
    vertices[0].vz = -10;
    vertices[1].vx = 10;
    vertices[1].vz = -10;
    vertices[2].vx = 10;
    vertices[2].vz = 10;
    vertices[3].vx = -10;
    vertices[3].vz = 10;
    normals[0].vy = 4096;
    index_id = addPtr(indices, 4);
    CHECK(index_id >= 0);
    geometry.numFaces = 1;
    geometry.pIndex = index_id;
    solid.flags = 1U << 2;
    solid.modelnode = &node;
    solid.geometry = &geometry;
    solid.coords = vertices;
    solid.normals = normals;

    scene.pScene = &scene.sceneRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent = &scene.sceneRoot;
    maPhysicsData[0].solid = &solid;
    numsolids = 1;
    CHECK(newclosestPoly(
              &from,
              &to,
              &move,
              10.0f,
              1.0f,
              &movenormal,
              2,
              &cubehit,
              &entryhit,
              &polyhit) ==
          1);
    CHECK(jpb_PhysicsGetWhichSolid() == &solid);
    CHECK(bestinfo.type == 1);
    CHECK(bestinfo.flags == 6);
    CHECK(fabsf(bestinfo.dist - 4.0f) < 0.00001f);
    CHECK(cubehit == NULL);
    CHECK(entryhit == NULL);
    CHECK(polyhit == NULL);

    pointerRegistry_Reset();
    leveldata = old_leveldata;
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    return 0;
}

static int test_world_blocking_core(void)
{
    int32_t map_storage[512] = {0};
    int32_t *old_leveldata = leveldata;
    int old_mapyend = mapyend;
    int old_numsolids = numsolids;
    float old_frame_rate = fGlobalFrameRate;
    playerObject player;
    physicsObject physics;
    wsl_ENEMY enemy;
    sceneObject scene;
    FVECTOR startpos = {0.0f, 20.0f, 0.0f};
    FVECTOR endpos = {1.0f, 2.0f, 3.0f};
    FVECTOR direction = {0.0f, -1.0f, 0.0f};

    memset(&player, 0, sizeof(player));
    memset(&physics, 0, sizeof(physics));
    memset(&enemy, 0, sizeof(enemy));
    memset(&scene, 0, sizeof(scene));
    CHECK(jpb_PhysicsWorldBlocking(
              NULL,
              &physics,
              &startpos,
              &endpos,
              &direction,
              9.0f) ==
          JPB_PHYSICS_PARTIAL_INVALID_ARGUMENT);

    physics.physicsRoot.objectID = 2;
    physics.mov.vx = 1.0f;
    player.pEnemy = &enemy;
    enemy.enemyFlags = UINT32_C(0xc400);
    CHECK(jpb_PhysicsWorldBlocking(
              &player,
              &physics,
              &startpos,
              &endpos,
              &direction,
              9.0f) == 0);

    player.pEnemy = NULL;
    physics.mov.vx = 0.0f;
    CHECK(jpb_PhysicsWorldBlocking(
              &player,
              &physics,
              &startpos,
              &endpos,
              &direction,
              9.0f) == 0);

    scene.pPhysics = &physics.physicsRoot;
    player.playerRoot.pParent = &scene.sceneRoot;
    physics.physicsRoot.objectID = 2;
    physics.mov.vx = 4.0f;
    physics.height = 20;
    physics.radius = 2;
    physics.maxledge = 32;
    physics.airGround = 77.0f;
    physics.flags = 0;
    player.pFlags = 0;
    mapyend = 0;
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    fGlobalFrameRate = 0.5f;
    CHECK(jpb_PhysicsWorldBlocking(
              &player,
              &physics,
              &startpos,
              &endpos,
              &direction,
              4.0f) == 1);
    CHECK((physics.flags & UINT32_C(0x800)) != 0);
    CHECK((player.pFlags & UINT32_C(0x100000)) != 0);
    leveldata = old_leveldata;
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    fGlobalFrameRate = old_frame_rate;
    return 0;
}

static int test_check_cube_blocking_no_contact(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    int old_mapyend = mapyend;
    int old_numsolids = numsolids;
    float old_frame_rate = fGlobalFrameRate;
    sceneObject scene;
    physicsObject physics;
    playerObject player;
    FVECTOR world = {0.0f, 0.0f, 0.0f};
    FVECTOR dir = {4.0f, 0.0f, 0.0f};
    FVECTOR normal = {1.0f, 0.0f, 0.0f};
    float ground = 123.0f;

    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&player, 0, sizeof(player));
    scene.pPhysics = &physics.physicsRoot;
    player.playerRoot.pParent = &scene.sceneRoot;
    physics.physicsRoot.objectID = 2;
    physics.height = 20;
    physics.radius = 2;
    physics.maxledge = 32;
    physics.airGround = 77.0f;
    mapyend = 0;
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    fGlobalFrameRate = 0.5f;

    CHECK(jpb_PhysicsCheckCubeBlocking(
              &player, &world, &dir, &normal, 4.0f, &ground) == 1);
    CHECK(world.vx == -4.0f);
    CHECK(world.vy == 0.0f);
    CHECK(world.vz == 0.0f);
    CHECK(ground == 77.0f);
    CHECK(physics.noncollideframes == 1);

    dir.vx = 4.0f;
    dir.vz = 3.0f;
    CHECK(jpb_PhysicsCheckCubeBlocking(
              &player, &world, &dir, &normal, 5.0f, &ground) == 1);
    CHECK(dir.vx == 0.0f);
    CHECK(dir.vz == 0.0f);
    CHECK(ground == 77.0f);

    leveldata = old_leveldata;
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    fGlobalFrameRate = old_frame_rate;
    return 0;
}

static int test_check_cube_blocking_dynamic_contact(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    int old_mapyend = mapyend;
    int old_numsolids = numsolids;
    int old_frame_rate = gGlobalFrameRate;
    sceneObject player_scene;
    sceneObject solid_scene;
    physicsObject physics;
    playerObject player;
    Mnode node;
    _solid solid;
    geomData geometry;
    _svector vertices[4];
    _svector normals[1];
    int16_t indices[4] = {0, 1, 2, 3};
    FVECTOR world = {0.0f, -5.0f, 0.0f};
    FVECTOR dir = {0.0f, -10.0f, 0.0f};
    FVECTOR normal = {0.0f, -1.0f, 0.0f};
    float ground = -32768.0f;
    int index_id;

    memset(&player_scene, 0, sizeof(player_scene));
    memset(&solid_scene, 0, sizeof(solid_scene));
    memset(&physics, 0, sizeof(physics));
    memset(&player, 0, sizeof(player));
    memset(&node, 0, sizeof(node));
    memset(&solid, 0, sizeof(solid));
    memset(&geometry, 0, sizeof(geometry));
    memset(vertices, 0, sizeof(vertices));
    memset(normals, 0, sizeof(normals));
    physics_gInitObjects(0);
    pointerRegistry_Reset();

    player_scene.pPhysics = &physics.physicsRoot;
    player.playerRoot.pParent = &player_scene.sceneRoot;
    player.pFlags = 1;
    physics.physicsRoot.objectID = 2;
    physics.radius = 1;
    physics.maxledge = 64;
    vertices[0].vx = -10;
    vertices[0].vz = -10;
    vertices[1].vx = 10;
    vertices[1].vz = -10;
    vertices[2].vx = 10;
    vertices[2].vz = 10;
    vertices[3].vx = -10;
    vertices[3].vz = 10;
    normals[0].vy = 4096;
    index_id = addPtr(indices, 4);
    CHECK(index_id >= 0);
    geometry.numFaces = 1;
    geometry.pIndex = index_id;
    solid.flags = 1U << 2;
    solid.modelnode = &node;
    solid.geometry = &geometry;
    solid.coords = vertices;
    solid.normals = normals;
    solid_scene.pScene = &solid_scene.sceneRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent =
        &solid_scene.sceneRoot;
    maPhysicsData[0].solid = &solid;
    numsolids = 1;
    mapyend = 0;
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    gGlobalFrameRate = 0x100;
    CHECK(jpb_PhysicsCheckCubeBlocking(
              &player, &world, &dir, &normal, 10.0f, &ground) == 1);
    CHECK(jpb_PhysicsGetWhichSolid() == &solid);
    CHECK((bestinfo.flags & 6) == 6);
    CHECK(physics.noncollideframes == 0);
    CHECK(physics.collidetime == UINT32_C(0x100));
    CHECK(physics.anycollidetime == UINT32_C(0x100));
    CHECK(world.vy > 4.9f);
    CHECK(world.vy < 5.1f);

    leveldata = old_leveldata;
    pointerRegistry_Reset();
    mapyend = old_mapyend;
    numsolids = old_numsolids;
    gGlobalFrameRate = old_frame_rate;
    return 0;
}

static int map_sound_calls;
static VECTOR map_sound_position;
static int map_sound_bank;
static char *map_sound_name;
static uint32_t map_sound_flag;

static uint16_t test_map_sound_hook(
    VECTOR *position,
    int bank,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    (void)user_data;
    ++map_sound_calls;
    map_sound_position = *position;
    map_sound_bank = bank;
    map_sound_name = sound;
    map_sound_flag = flag;
    return 1;
}

static void reset_map_effect_observations(void)
{
    map_sound_calls = 0;
    memset(&map_sound_position, 0, sizeof(map_sound_position));
    map_sound_bank = -1;
    map_sound_name = NULL;
    map_sound_flag = UINT32_MAX;
}

static int test_reset_jedi_complete(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    int old_numsolids = numsolids;
    char old_level = LevelSelect;
    gamestruct old_game = GameStruct;
    objectRoot actor;
    modelObject model;
    animObject animation;
    physicsObject *physics = &maPhysicsData[0];
    playerObject *player = &gaPlayerData[0];
    sceneObject *scene = &maSceneData[0];
    int index;

    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    player_gInitPlayers(0);
    connect_complete_actor(
        &actor, scene, &model, physics, &animation, player);
    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    LevelSelect = 1;
    memset(&GameStruct, 0, sizeof(GameStruct));
    GameStruct.NumPlayers = 2;

    physics->flags = UINT32_MAX;
    physics->movemode = MOVE_COREDEATH;
    physics->pos.vy = 1234.0f;
    physics->solidgrabbed = &maPhysicsData[1];
    physics->noncollideframes = 9;
    physics->collidetime = 10;
    physics->anycollidetime = 11;
    physics->reversoi = 12;
    physics->clipcode = 13;
    physics->hangcheck = 14;
    physics->standee = &maPhysicsData[1];
    physics->airTime = 15;
    physics->airmov.vx = 16.0f;
    physics->airmov.vy = 17.0f;
    physics->airmov.vz = 18.0f;
    player->pFlags = UINT32_MAX;
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        maPhysicsData[index].userdata[0] = index + 1;
    }

    physics_ResetJedi(0);
    CHECK(physics->flags == UINT32_C(0xffe4bfc0));
    CHECK(physics->movemode == MOVE_NORMAL);
    CHECK(physics->pos.vy == -32760.0f);
    CHECK(physics->solidgrabbed == NULL);
    CHECK(physics->noncollideframes == 0);
    CHECK(physics->collidetime == 0);
    CHECK(physics->anycollidetime == 0);
    CHECK(physics->reversoi == 0);
    CHECK(physics->clipcode == 0);
    CHECK(physics->hangcheck == 0);
    CHECK(physics->standee == NULL);
    CHECK(physics->airTime == 0);
    CHECK(physics->airmov.vx == 0.0f);
    CHECK(physics->airmov.vy == 0.0f);
    CHECK(physics->airmov.vz == 0.0f);
    CHECK(player->pFlags == UINT32_C(0xfffffffe));
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        CHECK(maPhysicsData[index].userdata[0] == 0);
    }

    LevelSelect = 10;
    GameStruct.NumPlayers = 2;
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        maPhysicsData[index].userdata[0] = index + 20;
        gaPlayerData[index].playerID =
            (int16_t)((index & 1) == 0 ? 0x4c : 0x4b);
    }
    physics_ResetJedi(0);
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        CHECK(
            maPhysicsData[index].userdata[0] ==
            ((index & 1) == 0 ? index + 20 : 0));
    }

    GameStruct.NumPlayers = 0;
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        maPhysicsData[index].userdata[0] = index + 40;
    }
    physics_ResetJedi(0);
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        CHECK(maPhysicsData[index].userdata[0] == index + 40);
    }

    GameStruct.NumPlayers = 1;
    physics_ResetJedi(0);
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        CHECK(maPhysicsData[index].userdata[0] == 0);
    }

    GameStruct = old_game;
    LevelSelect = old_level;
    numsolids = old_numsolids;
    leveldata = old_leveldata;
    return 0;
}

static int test_streets_ending_collision_trigger(void)
{
    int32_t map_storage[8] = {0};
    int32_t *old_leveldata = leveldata;
    int old_numsolids = numsolids;
    int old_totalframes = totalframes;
    char old_level = LevelSelect;
    gamestruct old_game = GameStruct;
    uint32_t old_cube_flags = jpb_CubeRuntimeFlags;
    uint8_t old_short_timeout =
        jpb_StreetsEndingShortCollisionTimeout;
    int32_t old_stapbikeindex[2] = {
        stapbikeindex[0], stapbikeindex[1]
    };
    uint16_t old_stapsound = stapsound;
    EffectHeader *old_effect = paEffects[18];
    EffectHeader effect;
    objectRoot actors[2];
    modelObject models[2];
    animObject animations[2];
    TestSoundStopCapture stop_capture;
    physicsObject *physics0 = &maPhysicsData[0];
    physicsObject *physics1 = &maPhysicsData[1];
    playerObject *player0 = &gaPlayerData[0];
    playerObject *player1 = &gaPlayerData[1];
    sceneObject *scene0 = &maSceneData[0];
    sceneObject *scene1 = &maSceneData[1];

    memset(&effect, 0, sizeof(effect));
    memset(&stop_capture, 0, sizeof(stop_capture));
    physics_gInitObjects(0);
    jpb_SceneInitPool(0);
    player_gInitPlayers(0);
    connect_complete_actor(
        &actors[0],
        scene0,
        &models[0],
        physics0,
        &animations[0],
        player0);
    connect_complete_actor(
        &actors[1],
        scene1,
        &models[1],
        physics1,
        &animations[1],
        player1);
    actors[1].objectID = 1;
    models[1].modelRoot.objectID = 1;
    physics1->physicsRoot.objectID = 1;
    animations[1].animRoot.objectID = 1;
    player1->playerRoot.objectID = 1;

    leveldata = map_storage + 4;
    leveldata[-2] = 0;
    numsolids = 0;
    LevelSelect = 8;
    totalframes = 0x21;
    memset(&GameStruct, 0, sizeof(GameStruct));
    GameStruct.NumPlayers = 1;
    jpb_CubeRuntimeFlags = 0;
    jpb_StreetsEndingShortCollisionTimeout = 0;
    stapbikeindex[0] = 1;
    stapbikeindex[1] = 0;
    stapsound = 77;
    paEffects[18] = &effect;
    physics0->pos.vx = 10.0f;
    physics0->pos.vy = 20.0f;
    physics0->pos.vz = 30.0f;
    physics0->vpos.vx = 10;
    physics0->vpos.vy = 20;
    physics0->vpos.vz = 30;
    physics0->anycollidetime = UINT32_C(0x0001e000);
    physics0->userdata[0] = 5;
    physics1->userdata[0] = 6;
    bestinfo.flags = 0;
    bestinfo.facenormal.vx = 0.9f;
    jpb_PhysicsSetStreetsEndingCountdown(0);
    reset_map_effect_observations();
    jpb_SoundSetPlaySfxHook(test_map_sound_hook, NULL);
    jpb_SoundSetStopHook(
        test_sound_stop_hook, &stop_capture);

    CHECK(jpb_PhysicsTryStartStreetsEnding(
              physics0, 1) == 0);
    CHECK(jpb_PhysicsGetStreetsEndingCountdown() == 0);
    jpb_StreetsEndingShortCollisionTimeout = 1;
    CHECK(jpb_PhysicsTryStartStreetsEnding(
              physics0, 1) == 1);
    CHECK(jpb_PhysicsGetStreetsEndingCountdown() == 0x13800);
    CHECK((jpb_CubeRuntimeFlags & UINT32_C(8)) != 0);
    CHECK((actors[0].flags & UINT32_C(0x20)) != 0);
    CHECK(stapbikeindex[0] == 0);
    CHECK(stapbikeindex[1] == 0);
    CHECK(stapsound == 0);
    CHECK(stop_capture.count == 1);
    CHECK(stop_capture.handles[0] == 77);
    CHECK(map_sound_calls == 1);
    CHECK(map_sound_bank == 0);
    CHECK(strcmp(map_sound_name, "explomed") == 0);
    CHECK(map_sound_flag == 0);
    CHECK(physics0->userdata[0] == 0);
    CHECK(physics1->userdata[0] == 0);
    CHECK(physics0->anycollidetime == 0);
    CHECK(physics0->collidetime == 0);
    CHECK(player0->pFlags == 0);
    CHECK(player1->pFlags == 0);
    CHECK(jpb_PhysicsTryStartStreetsEnding(
              physics0, 1) == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    jpb_SoundSetStopHook(NULL, NULL);
    paEffects[18] = old_effect;
    stapsound = old_stapsound;
    stapbikeindex[0] = old_stapbikeindex[0];
    stapbikeindex[1] = old_stapbikeindex[1];
    jpb_StreetsEndingShortCollisionTimeout =
        old_short_timeout;
    jpb_CubeRuntimeFlags = old_cube_flags;
    GameStruct = old_game;
    totalframes = old_totalframes;
    LevelSelect = old_level;
    numsolids = old_numsolids;
    leveldata = old_leveldata;
    jpb_PhysicsSetStreetsEndingCountdown(0);
    return 0;
}

static int test_launch_splash(void)
{
    int32_t map_storage[260] = {0};
    int32_t *old_leveldata = leveldata;
    int old_numsolids = numsolids;
    EffectHeader effect;
    EffectHeader *old_effect = paEffects[61];
    _Material material;
    _Material *old_material = effects1Handle[0];
    char old_level = LevelSelect;
    uint32_t old_game_state = GameStruct.GameState;
    uint8_t old_fun_factor = OptionStruct.FunFactor;
    physicsObject physics;
    SCB *scb;

    memset(&effect, 0, sizeof(effect));
    memset(&material, 0, sizeof(material));
    memset(&physics, 0, sizeof(physics));
    effect.num = 1;
    effect.aEffects[0].bank = 1;
    effect.aEffects[0].type = 0;
    material.iw = 8;
    material.ih = 6;
    paEffects[61] = &effect;
    effects1Handle[0] = &material;
    LevelSelect = 1;
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    leveldata = map_storage + 4;
    leveldata[-2] = 1 << 10;
    numsolids = 0;
    physics.pos.vx = 32768.0f;
    physics.pos.vy = 100.0f;
    physics.pos.vz = -32512.0f;
    meminit();
    sprite_gInitSprites();
    reset_map_effect_observations();
    jpb_SoundSetPlaySfxHook(test_map_sound_hook, NULL);

    jpb_PhysicsLaunchSplash(&physics);
    CHECK(physics.airTime == 0xf000);
    CHECK(mSCBDraw[0].head != NULL);
    scb = (SCB *)mSCBDraw[0].head;
    CHECK(scb->scb_Texture == &material);
    CHECK(scb->scb_vertex0.vx == 32764.0f);
    CHECK(scb->scb_vertex0.vy == -32763.0f);
    CHECK(scb->scb_vertex0.vz == -32512.0f);
    CHECK(map_sound_calls == 1);
    CHECK(map_sound_bank == 3);
    CHECK(strcmp(map_sound_name, "splash") == 0);
    CHECK(map_sound_flag == 0);
    CHECK(map_sound_position.vx == 32768);
    CHECK(map_sound_position.vy == 100);
    CHECK(map_sound_position.vz == -32512);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    paEffects[61] = old_effect;
    effects1Handle[0] = old_material;
    LevelSelect = old_level;
    GameStruct.GameState = old_game_state;
    OptionStruct.FunFactor = old_fun_factor;
    leveldata = old_leveldata;
    numsolids = old_numsolids;
    return 0;
}

static int test_target_facing_helpers(void)
{
    objectRoot player = {0};
    objectRoot target = {0};
    sceneObject player_scene = {0};
    sceneObject target_scene = {0};
    physicsObject player_physics = {0};
    physicsObject target_physics = {0};

    player.pParent = &player_scene.sceneRoot;
    target.pParent = &target_scene.sceneRoot;
    player_scene.pPhysics = &player_physics.physicsRoot;
    target_scene.pPhysics = &target_physics.physicsRoot;
    player_physics.vpos.vx = 10;
    player_physics.vpos.vz = 20;
    target_physics.vpos.vx = 110;
    target_physics.vpos.vz = 20;

    CHECK(physics_gFaceTarget(&player, &target) == 1023);
    CHECK(physics_gFaceTarget(&player, NULL) == 0);
    CHECK(physics_gFaceTarget(NULL, &target) == 0);
    player_physics.face.vy = -1;
    CHECK(physics_ForceFaceLock(&player, &target) == 0);
    CHECK(player_physics.face.vy == 1023);
    player_physics.face.vy = -1;
    CHECK(physics_ForceFaceLock(&player, NULL) == 0);
    CHECK(player_physics.face.vy == 0);

    player_physics.angle.vy = 3000;
    CHECK(
        physics_gGetFaceTargetDelta(&player, &target) ==
        1977);
    CHECK(physics_gForceFaceTarget(&player, &target) == 0);
    CHECK(player_physics.angle.vy == 1023);

    player_physics.flags = UINT32_C(0x400000);
    player_physics.angle.vy = 333;
    CHECK(physics_gForceFaceTarget(&player, &target) == 0);
    CHECK(player_physics.angle.vy == 333);

    target_scene.pPhysics = NULL;
    CHECK(physics_gFaceTarget(&player, &target) == 0);
    CHECK(physics_gGetFaceTargetDelta(&player, &target) == 333);
    CHECK(physics_gForceFaceTarget(&player, &target) == 0);
    CHECK(player_physics.angle.vy == 333);

    player_scene.pPhysics = NULL;
    CHECK(physics_gFaceTarget(&player, &target) == 0);
    CHECK(physics_gGetFaceTargetDelta(&player, &target) == 0);
    CHECK(physics_gForceFaceTarget(&player, &target) == 0);
    return 0;
}

static int test_ground_target_and_object_owners(void)
{
    int32_t map_words[4] = {0};
    int32_t polygon = 1;
    int32_t *old_leveldata = leveldata;
    sceneObject *scene;
    physicsObject *physics;
    playerObject *player0 = &gaPlayerData[0];
    playerObject *player1 = &gaPlayerData[1];
    VECTOR target_offset = {11, 22, 33, 0x12345678};
    int row;
    int column;

    for (row = 0; row < JPB_PHYSICS_CAPACITY; ++row) {
        for (column = 0;
             column < JPB_PHYSICS_CAPACITY;
             ++column) {
            maRange[row][column] = 12.0f;
        }
    }
    physics_InitPhysics();
    for (row = 0; row < JPB_PHYSICS_CAPACITY; ++row) {
        for (column = 0;
             column < JPB_PHYSICS_CAPACITY;
             ++column) {
            CHECK(maRange[row][column] == -1.0f);
        }
    }
    CHECK(physics_MapAnimCallBack(NULL, NULL) == 0);

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    scene = scene_gGetNewSceneObject(4);
    CHECK(scene == &maSceneData[4]);
    physics = physics_gCreateObject(scene);
    CHECK(physics == &maPhysicsData[4]);
    CHECK(scene->pPhysics == &physics->physicsRoot);
    CHECK(physics->physicsRoot.pParent == &scene->sceneRoot);
    CHECK(physics->maxledge == 0x10000);
    physics->lastpolyhit = &polygon;
    CHECK(physics_GetPoly(&physics->physicsRoot) == &polygon);

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    maSceneData[0].sceneRoot.objectID = 0;
    maSceneData[0].pScene = &maSceneData[0].sceneRoot;
    maSceneData[0].pPhysics = &maPhysicsData[0].physicsRoot;
    maSceneData[0].pPlayer = &player0->playerRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent =
        &maSceneData[0].sceneRoot;
    player0->playerRoot.objectID = 0;
    player0->playerRoot.pParent = &maSceneData[0].sceneRoot;
    maSceneData[1].sceneRoot.objectID = 1;
    maSceneData[1].pScene = &maSceneData[1].sceneRoot;
    maSceneData[1].pPhysics = &maPhysicsData[1].physicsRoot;
    maSceneData[1].pPlayer = &player1->playerRoot;
    maPhysicsData[1].physicsRoot.objectID = 1;
    maPhysicsData[1].physicsRoot.pParent =
        &maSceneData[1].sceneRoot;
    player1->playerRoot.objectID = 1;
    player1->playerRoot.pParent = &maSceneData[1].sceneRoot;

    leveldata = map_words;
    maPhysicsData[0].lastpolyhit = &polygon;
    map_words[1] = INT32_C(0x20000);
    CHECK(physics_gCheckGround(player0) == 1);
    map_words[1] = 0;
    maPhysicsData[0].pos.vy = 132.9f;
    maPhysicsData[0].airGround = 100.0f;
    CHECK(physics_gCheckGround(player0) == 0);
    maPhysicsData[0].pos.vy = 133.0f;
    CHECK(physics_gCheckGround(player0) == 1);
    player0->pFlags = UINT32_C(1);
    maPhysicsData[0].radius = 8;
    maPhysicsData[0].pos.vy = 131.0f;
    CHECK(physics_gCheckGround(player0) == 1);
    player0->pFlags = 0;

    maPhysicsData[0].angle = (VECTOR){0, 0, 0, 0};
    maPhysicsData[0].pos =
        (FVECTOR){100.75f, 200.25f, -50.9f};
    maPhysicsData[0].constmov =
        (FVECTOR){4.0f, 5.0f, 6.0f};
    maPhysicsData[1].mov =
        (FVECTOR){1.5f, -2.0f, 3.9f};
    maPhysicsData[1].constmov =
        (FVECTOR){7.0f, 8.0f, 9.0f};
    physics_gCalcTargetPos(0, &target_offset);
    CHECK(target_offset.vx == 109);
    CHECK(target_offset.vy == 224);
    CHECK(target_offset.vz == -21);
    CHECK(target_offset.pad == 0x12345678);
    CHECK(maPhysicsData[1].pos.vx == 109.0f);
    CHECK(maPhysicsData[1].pos.vy == 224.0f);
    CHECK(maPhysicsData[1].pos.vz == -21.0f);
    CHECK(maPhysicsData[0].constmov.vx == 0.0f);
    CHECK(maPhysicsData[0].constmov.vy == 0.0f);
    CHECK(maPhysicsData[0].constmov.vz == 0.0f);
    CHECK(maPhysicsData[1].constmov.vx == 0.0f);
    CHECK(maPhysicsData[1].constmov.vy == 0.0f);
    CHECK(maPhysicsData[1].constmov.vz == 0.0f);
    CHECK(maSceneData[1].v3WorldPosition.vx == 109);
    CHECK(maSceneData[1].v3WorldPosition.vy == 224);
    CHECK(maSceneData[1].v3WorldPosition.vz == -21);
    leveldata = old_leveldata;
    return 0;
}

static int test_nearest_physics_target(void)
{
    wsl_ENEMY enemies[4];
    wsl_BAP_PLACEMENT placements[4];
    const int candidate_ids[4] = {2, 3, 4, 5};
    int index;

    memset(enemies, 0, sizeof(enemies));
    memset(placements, 0, sizeof(placements));
    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    physics_InitPhysics();

    maSceneData[0].sceneRoot.objectID = 0;
    maSceneData[0].pScene = &maSceneData[0].sceneRoot;
    maSceneData[0].pPhysics = &maPhysicsData[0].physicsRoot;
    maPhysicsData[0].physicsRoot.objectID = 0;
    maPhysicsData[0].physicsRoot.pParent =
        &maSceneData[0].sceneRoot;

    for (index = 0; index < 4; ++index) {
        int id = candidate_ids[index];
        playerObject *player = &gaPlayerData[id];

        maSceneData[id].sceneRoot.objectID = id;
        maSceneData[id].pScene = &maSceneData[id].sceneRoot;
        maSceneData[id].pPhysics =
            &maPhysicsData[id].physicsRoot;
        maSceneData[id].pPlayer = &player->playerRoot;
        maPhysicsData[id].physicsRoot.objectID = id;
        maPhysicsData[id].physicsRoot.pParent =
            &maSceneData[id].sceneRoot;
        player->playerRoot.objectID = id;
        player->playerRoot.pParent =
            &maSceneData[id].sceneRoot;
        player->pEnemy = &enemies[index];
        enemies[index].pPlace = &placements[index];
        placements[index].aiDf.daDelay = 7;
    }
    placements[3].aiDf.daDelay = 8;
    maSceneData[4].sceneRoot.flags = UINT32_C(0x20);
    maRange[0][2] = 500.0f;
    maRange[0][3] = 100.0f;
    maRange[0][4] = 25.0f;
    maRange[0][5] = 10.0f;

    CHECK(physics_gGetNearestTarget(
              &maPhysicsData[0].physicsRoot, 7) ==
          &maPhysicsData[3].physicsRoot);
    maSceneData[3].sceneRoot.flags = UINT32_C(0x20);
    CHECK(physics_gGetNearestTarget(
              &maPhysicsData[0].physicsRoot, 7) ==
          &maPhysicsData[2].physicsRoot);
    maRange[0][2] = 0.0f;
    CHECK(physics_gGetNearestTarget(
              &maPhysicsData[0].physicsRoot, 7) == NULL);
    return 0;
}

static int test_launch_map_anim_effects(void)
{
    EffectHeader effect;
    EffectHeader *old_effect = paEffects[10];
    _Material material;
    _Material *old_material = effects1Handle[0];
    playerObject old_players[JPB_PLAYER_CAPACITY - 2];
    physicsObject old_physics[JPB_PLAYER_CAPACITY - 2];
    char old_level = LevelSelect;
    uint32_t old_game_state = GameStruct.GameState;
    uint8_t old_fun_factor = OptionStruct.FunFactor;
    VECTOR worldpos = {11, 22, 33, 0};
    int32_t events[2] = {
        (int32_t)(
            (UINT32_C(1) << 24) |
            (UINT32_C(3) << 16) |
            (UINT32_C(4) << 8) |
            UINT32_C(2)),
        (int32_t)(UINT32_C(14) << 24)
    };
    int i;
    Node *last_effect_scb;

    memset(&effect, 0, sizeof(effect));
    memset(&material, 0, sizeof(material));
    effect.num = 2;
    effect.aEffects[0].bank = 1;
    effect.aEffects[0].type = 0;
    material.iw = 8;
    material.ih = 6;
    paEffects[10] = &effect;
    effects1Handle[0] = &material;
    jpb_SoundSetPlaySfxHook(test_map_sound_hook, NULL);
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    meminit();
    sprite_gInitSprites();

    reset_map_effect_observations();
    cullmesh[0] = 1;
    cullmesh[1] = 0;
    LevelSelect = 1;
    LaunchMapAnimEffects(2, &worldpos, events);
    CHECK(mSCBDraw[0].head != NULL);
    CHECK(((SCB *)mSCBDraw[0].head)->scb_Texture == &material);
    CHECK(((SCB *)mSCBDraw[0].head)->scb_vertex0.vx == 7.0f);
    CHECK(((SCB *)mSCBDraw[0].head)->scb_vertex0.vy == 19.0f);
    CHECK(((SCB *)mSCBDraw[0].head)->scb_vertex0.vz == 33.0f);
    CHECK(map_sound_calls == 1);
    CHECK(map_sound_bank == 3);
    CHECK(strcmp(map_sound_name, "explosm") == 0);
    CHECK(map_sound_flag == 0);
    CHECK(map_sound_position.vx == 0x7e80);
    CHECK(map_sound_position.vy == 0x380);
    CHECK(map_sound_position.vz == 0x8580);
    CHECK(cullmesh[0] == 0);
    CHECK(cullmesh[1] == 1);
    last_effect_scb = mSCBDraw[0].tail;

    reset_map_effect_observations();
    LevelSelect = 4;
    LaunchMapAnimEffects(2, &worldpos, events);
    CHECK(mSCBDraw[0].tail == last_effect_scb);
    CHECK(map_sound_calls == 0);

    reset_map_effect_observations();
    events[0] = (int32_t)(UINT32_C(2) << 24);
    cullmesh[3] = 0;
    cullmesh[4] = 1;
    LevelSelect = 8;
    LaunchMapAnimEffects(1, &worldpos, events);
    CHECK(mSCBDraw[0].tail != last_effect_scb);
    CHECK(cullmesh[3] == 1);
    CHECK(cullmesh[4] == 0);

    memcpy(
        old_players,
        &gaPlayerData[2],
        sizeof(old_players));
    memcpy(
        old_physics,
        &maPhysicsData[2],
        sizeof(old_physics));
    memset(
        &gaPlayerData[2],
        0,
        sizeof(old_players));
    memset(
        &maPhysicsData[2],
        0,
        sizeof(old_physics));
    for (i = 2; i < JPB_PLAYER_CAPACITY; ++i) {
        gaPlayerData[i].playerRoot.objectID = -1;
    }
    gaPlayerData[2].playerRoot.objectID = 0;
    gaPlayerData[2].playerID = 0x4c;
    maPhysicsData[2].pos.vx = 10.0f;
    gaPlayerData[3].playerRoot.objectID = 0;
    gaPlayerData[3].playerID = 0x4c;
    maPhysicsData[3].pos.vx = 20.0f;
    events[0] = (int32_t)(UINT32_C(3) << 24);
    LevelSelect = 10;
    LaunchMapAnimEffects(1, &worldpos, events);
    CHECK(maPhysicsData[2].userdata[0] == 1);
    CHECK(maPhysicsData[3].userdata[0] == 0);
    memcpy(
        &gaPlayerData[2],
        old_players,
        sizeof(old_players));
    memcpy(
        &maPhysicsData[2],
        old_physics,
        sizeof(old_physics));

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    paEffects[10] = old_effect;
    effects1Handle[0] = old_material;
    LevelSelect = old_level;
    GameStruct.GameState = old_game_state;
    OptionStruct.FunFactor = old_fun_factor;
    return 0;
}

static int test_map_event_tables(void)
{
    static const char *const expected_sounds[16] = {
        "explosm",
        "break1",
        "break2",
        "walbreak",
        "lgspark",
        "explomed",
        "glasbrk1",
        "trap",
        "walbreak",
        "brk_pole",
        "trap",
        "beam_on",
        "lasrgate",
        "forcefld1",
        "explosm",
        "sabrhit7"
    };
    int level;
    int event;

    for (event = 0; event < 15; ++event) {
        CHECK(eventarray[0][event] == 0);
    }
    for (level = 1; level < 30; ++level) {
        CHECK(eventarray[level][0] == 0);
        CHECK(eventarray[level][14] == 0);
        for (event = 1; event < 14; ++event) {
            uint16_t expected =
                level == 4 && event == 1
                    ? UINT16_C(0x0e1e)
                    : UINT16_C(0x0e0a);

            CHECK(eventarray[level][event] == expected);
        }
    }
    for (event = 0; event < 16; ++event) {
        CHECK(strcmp(maphitsounds[event], expected_sounds[event]) == 0);
    }
    return 0;
}

static int test_snapshot_position(void)
{
    sceneObject *scene;
    physicsObject *physics;
    playerObject player;
    modelObject model;
    Mnode pelvis;

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    coll_ResetCollisionSystem();
    scene = &maSceneData[0];
    physics = &maPhysicsData[0];
    memset(&player, 0, sizeof(player));
    memset(&model, 0, sizeof(model));
    memset(&pelvis, 0, sizeof(pelvis));

    scene->sceneRoot.objectID = 0;
    scene->pScene = &scene->sceneRoot;
    scene->pModel = &model.modelRoot;
    scene->pPhysics = &physics->physicsRoot;
    scene->pPlayer = &player.playerRoot;
    physics->physicsRoot.objectID = 0;
    physics->physicsRoot.pParent = &scene->sceneRoot;
    player.playerRoot.objectID = 0;
    player.playerRoot.pParent = &scene->sceneRoot;
    model.modelRoot.objectID = 0;
    model.modelRoot.pParent = &scene->sceneRoot;

    scene->v3WorldPosition.vx = 10;
    scene->v3WorldPosition.vy = 20;
    scene->v3WorldPosition.vz = 30;
    scene->v3SnapShotPosition.vx = 100;
    scene->v3SnapShotPosition.vy = 200;
    scene->v3SnapShotPosition.vz = 300;
    physics_gSnapShotPosition(&player.playerRoot, 60);
    CHECK(physics->pos.vx == 10.0f);
    CHECK(physics->pos.vy == 20.0f);
    CHECK(physics->pos.vz == 30.0f);
    CHECK(physics->snapshotpos.vx == 100.0f);
    CHECK(physics->snapshotpos.vy == 260.0f);
    CHECK(physics->snapshotpos.vz == 300.0f);

    player.playerID = 2;
    model.flags = UINT32_C(0x20);
    pelvis.id = NODE_DYNAMIC;
    pelvis.v3RotCenter.vy = 777;
    coll_gRegisterNode(player.playerID, &pelvis);
    physics_gSnapShotPosition(&player.playerRoot, 60);
    CHECK(physics->snapshotpos.vy == 777.0f);

    player.pFlags = UINT32_C(0x2000);
    physics->pos.vx = -1.0f;
    physics->snapshotpos.vy = -2.0f;
    physics_gSnapShotPosition(&player.playerRoot, 60);
    CHECK(physics->pos.vx == -1.0f);
    CHECK(physics->snapshotpos.vy == -2.0f);

    player.pFlags = 0;
    scene->sceneRoot.flags = UINT32_C(0x20);
    physics->pos.vx = -3.0f;
    physics_gSnapShotPosition(&player.playerRoot, 60);
    CHECK(physics->pos.vx == -3.0f);
    return 0;
}

static int test_character_blocking_kernel(void)
{
    playerObject player0;
    playerObject player1;
    sceneObject scene1;
    physicsObject p0;
    physicsObject p1;
    FVECTOR testpos0 = {0.0f, 0.0f, 0.0f};
    float range = -1.0f;

    memset(&player0, 0, sizeof(player0));
    memset(&player1, 0, sizeof(player1));
    memset(&scene1, 0, sizeof(scene1));
    memset(&p0, 0, sizeof(p0));
    memset(&p1, 0, sizeof(p1));
    p1.physicsRoot.pParent = &scene1.sceneRoot;
    scene1.pPlayer = &player1.playerRoot;
    p0.radius = 10;
    p1.radius = 10;
    p0.height = 20;
    p1.height = 20;
    p0.mass = 10;
    p1.mass = 30;
    p1.pos.vx = 5.0f;

    CHECK(jpb_PhysicsCharBlockingState(
              &player0,
              &p0,
              &p1,
              &testpos0,
              &range) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(range - 5.0f) < 0.00001f);
    CHECK(fabsf(p0.mov.vx - -11.25f) < 0.00001f);
    CHECK(fabsf(p1.mov.vx - 3.75f) < 0.00001f);
    CHECK(p0.mov.vy == 0.0f);
    CHECK(p1.mov.vy == 0.0f);
    CHECK(p0.mov.vz == 0.0f);
    CHECK(p1.mov.vz == 0.0f);
    CHECK((p0.flags & UINT32_C(0x80800)) ==
          UINT32_C(0x80800));
    CHECK((p1.flags & UINT32_C(0x80800)) ==
          UINT32_C(0x80800));

    memset(&p0.mov, 0, sizeof(p0.mov));
    memset(&p1.mov, 0, sizeof(p1.mov));
    p0.flags = 0;
    p1.flags = 0;
    p0.mass = INT16_MAX;
    p1.mass = 10;
    p0.radius = 100;
    p1.radius = 100;
    p1.pos.vx = 1.0f;
    CHECK(jpb_PhysicsCharBlockingState(
              &player0,
              &p0,
              &p1,
              &testpos0,
              &range) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(p0.mov.vx == 0.0f);
    CHECK(p1.mov.vx == 48.0f);

    memset(&p0.mov, 0, sizeof(p0.mov));
    memset(&p1.mov, 0, sizeof(p1.mov));
    p0.flags = 0;
    p1.flags = UINT32_C(0x20);
    p0.mass = 10;
    p1.mass = 30;
    p0.radius = 10;
    p1.radius = 10;
    p1.pos.vx = 5.0f;
    CHECK(jpb_PhysicsCharBlockingState(
              &player0,
              &p0,
              &p1,
              &testpos0,
              &range) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(p0.mov.vx == 0.0f);
    CHECK(p1.mov.vx == 0.0f);
    CHECK(p0.flags == 0);
    CHECK(p1.flags == UINT32_C(0x20));

    p1.flags = 0;
    p1.pos.vy = 100.0f;
    CHECK(jpb_PhysicsCharBlockingState(
              &player0,
              &p0,
              &p1,
              &testpos0,
              &range) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(range - sqrtf(10025.0f)) < 0.00001f);
    CHECK(p0.mov.vx == 0.0f);
    CHECK(p1.mov.vx == 0.0f);
    return 0;
}

static int test_move_character_contact_sweep(void)
{
    sceneObject *scene0;
    sceneObject *scene1;
    physicsObject *p0;
    physicsObject *p1;
    playerObject *player0;
    playerObject *player1;
    int index;

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    scene0 = &maSceneData[0];
    scene1 = &maSceneData[1];
    p0 = &maPhysicsData[0];
    p1 = &maPhysicsData[1];
    player0 = &gaPlayerData[0];
    player1 = &gaPlayerData[1];

    scene0->sceneRoot.objectID = 0;
    scene0->pScene = &scene0->sceneRoot;
    scene0->pPhysics = &p0->physicsRoot;
    scene0->pPlayer = &player0->playerRoot;
    p0->physicsRoot.objectID = 0;
    p0->physicsRoot.pParent = &scene0->sceneRoot;
    player0->playerRoot.objectID = 0;
    player0->playerRoot.pParent = &scene0->sceneRoot;

    scene1->sceneRoot.objectID = 1;
    scene1->pScene = &scene1->sceneRoot;
    scene1->pPhysics = &p1->physicsRoot;
    scene1->pPlayer = &player1->playerRoot;
    p1->physicsRoot.objectID = 1;
    p1->physicsRoot.pParent = &scene1->sceneRoot;
    player1->playerRoot.objectID = 1;
    player1->playerRoot.pParent = &scene1->sceneRoot;

    p0->radius = 10;
    p1->radius = 10;
    p0->height = 20;
    p1->height = 20;
    p0->mass = 10;
    p1->mass = 30;
    p1->pos.vx = 5.0f;
    for (index = 0; index < JPB_PHYSICS_CAPACITY; ++index) {
        maRange[0][index] = 123.0f;
    }

    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(fabsf(maRange[0][1] - 5.0f) < 0.00001f);
    for (index = 2; index < JPB_PHYSICS_CAPACITY; ++index) {
        CHECK(maRange[0][index] == -1.0f);
    }
    CHECK(fabsf(p0->mov.vx - -11.25f) < 0.00001f);
    CHECK(fabsf(p1->mov.vx - 3.75f) < 0.00001f);

    memset(&p0->mov, 0, sizeof(p0->mov));
    memset(&p1->mov, 0, sizeof(p1->mov));
    p0->flags = 0;
    p1->flags = 0;
    player1->pFlags = UINT32_C(0x0800);
    maRange[0][1] = 123.0f;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(maRange[0][1] == -1.0f);
    CHECK(p0->mov.vx == 0.0f);
    CHECK(p1->mov.vx == 0.0f);

    player1->pFlags = 0;
    p1->flags = UINT32_C(0x00800000);
    player1->playerID = 12;
    maRange[0][1] = 123.0f;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(maRange[0][1] == -1.0f);

    p1->flags = 0;
    player0->pFlags = UINT32_C(0x04000000);
    maRange[0][1] = 123.0f;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(maRange[0][1] == 123.0f);
    return 0;
}

static int test_move_large_character_node_contacts(void)
{
    sceneObject *scene0;
    sceneObject *scene1;
    physicsObject *p0;
    physicsObject *p1;
    playerObject *player0;
    playerObject *player1;
    Mnode nodes[6];
    int index;

    static const int desert_node_ids[4] = {
        0x00, 0x0e, 0x14, 0x15
    };
    static const int worm_node_ids[6] = {
        0x00, 0x0e, 0x0f, 0x17, 0x19, 0x1a
    };

    CHECK(maDesert_BNodeSizes[0].radius1 == 0x100);
    CHECK(maDesert_BNodeSizes[0].id == 0);
    CHECK(maDesert_BNodeSizes[2].radius1 == 0x80);
    CHECK(maDesert_BNodeSizes[3].id == 0x15);
    CHECK(maWormNodeSizes[0].id == 0);
    CHECK(maWormNodeSizes[2].radius1 == 0x100);
    CHECK(maWormNodeSizes[4].id == 0x19);
    CHECK(maWormNodeSizes[5].radius1 == 0x66);

    jpb_SceneInitPool(0);
    physics_gInitObjects(0);
    player_gInitPlayers(0);
    coll_ResetCollisionSystem();
    scene0 = &maSceneData[0];
    scene1 = &maSceneData[1];
    p0 = &maPhysicsData[0];
    p1 = &maPhysicsData[1];
    player0 = &gaPlayerData[0];
    player1 = &gaPlayerData[1];

    scene0->sceneRoot.objectID = 0;
    scene0->pScene = &scene0->sceneRoot;
    scene0->pPhysics = &p0->physicsRoot;
    scene0->pPlayer = &player0->playerRoot;
    p0->physicsRoot.objectID = 0;
    p0->physicsRoot.pParent = &scene0->sceneRoot;
    player0->playerRoot.objectID = 0;
    player0->playerRoot.pParent = &scene0->sceneRoot;

    scene1->sceneRoot.objectID = 1;
    scene1->pScene = &scene1->sceneRoot;
    scene1->pPhysics = &p1->physicsRoot;
    scene1->pPlayer = &player1->playerRoot;
    p1->physicsRoot.objectID = 1;
    p1->physicsRoot.pParent = &scene1->sceneRoot;
    player1->playerRoot.objectID = 1;
    player1->playerRoot.pParent = &scene1->sceneRoot;

    p0->radius = 10;
    p0->pos.vx = 20.0f;
    p0->pos.vy = 10.0f;
    p1->flags = UINT32_C(0x00800000);
    player1->playerID = JPB_PLAYER_ID_DESERT_BEAST;

    memset(nodes, 0, sizeof(nodes));
    for (index = 0; index < 4; ++index) {
        nodes[index].id =
            (modelNodeId)(
                NODE_DYNAMIC |
                (unsigned)desert_node_ids[index]);
        nodes[index].v3RotCenter.vx = 1000;
        nodes[index].v3RotCenter.vy = 10;
        coll_gRegisterNode(1, &nodes[index]);
    }
    nodes[2].v3RotCenter.vx = 10;
    maRange[0][1] = 123.0f;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(maRange[0][1] == -1.0f);
    CHECK(fabsf(p0->mov.vx - 128.0f) < 0.00001f);
    CHECK(p0->mov.vy == 0.0f);
    CHECK(p0->mov.vz == 0.0f);
    CHECK((p0->flags & UINT32_C(0x80800)) ==
          UINT32_C(0x80800));
    CHECK(p1->mov.vx == 0.0f);

    memset(&p0->mov, 0, sizeof(p0->mov));
    p0->flags = 0;
    nodes[2].flags = 1;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(p0->mov.vx == 0.0f);
    CHECK(p0->flags == 0);

    nodes[2].flags = 0;
    nodes[2].v3RotCenter.vy = -119;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(p0->mov.vx == 0.0f);
    CHECK(p0->flags == 0);

    coll_ResetCollisionSystem();
    memset(nodes, 0, sizeof(nodes));
    for (index = 0; index < 6; ++index) {
        int node_id = worm_node_ids[index];

        nodes[index].id =
            (modelNodeId)(NODE_DYNAMIC | (unsigned)node_id);
        nodes[index].v3RotCenter.vx = 1000;
        nodes[index].v3RotCenter.vy = 10;
        coll_gRegisterNode(1, &nodes[index]);
    }
    nodes[4].v3RotCenter.vx = 12;
    memset(&p0->mov, 0, sizeof(p0->mov));
    p0->flags = 0;
    player1->playerID = JPB_PLAYER_ID_WORM;
    CHECK(jpb_PhysicsMoveCharacterContacts(p0) ==
          JPB_PHYSICS_PARTIAL_OK);
    CHECK(maRange[0][1] == -1.0f);
    CHECK(fabsf(p0->mov.vx - 104.0f) < 0.00001f);
    CHECK(p0->mov.vy == 0.0f);
    CHECK(p0->mov.vz == 0.0f);
    CHECK((p0->flags & UINT32_C(0x80800)) ==
          UINT32_C(0x80800));
    return 0;
}

static int test_range_cache_and_fallback(void)
{
    sceneObject scene0;
    sceneObject scene1;
    physicsObject p0;
    physicsObject p1;

    memset(&scene0, 0, sizeof(scene0));
    memset(&scene1, 0, sizeof(scene1));
    memset(&p0, 0, sizeof(p0));
    memset(&p1, 0, sizeof(p1));
    p0.physicsRoot.objectID = 2;
    p1.physicsRoot.objectID = 5;
    p0.physicsRoot.pParent = &scene0.sceneRoot;
    p1.physicsRoot.pParent = &scene1.sceneRoot;
    scene0.pPhysics = &p0.physicsRoot;
    scene1.pPhysics = &p1.physicsRoot;

    maRange[2][5] = 77.9f;
    CHECK(physics_gGetRange(
              &p0.physicsRoot,
              &p1.physicsRoot) == 77);
    CHECK(physics_gGetRange(
              &p1.physicsRoot,
              &p0.physicsRoot) == 77);

    maRange[2][5] = -1.0f;
    p0.vpos.vx = 0;
    p0.vpos.vy = 0;
    p0.vpos.vz = 0;
    p1.vpos.vx = 3;
    p1.vpos.vy = 4;
    p1.vpos.vz = 12;
    CHECK(physics_gGetRange(
              &p0.physicsRoot,
              &p1.physicsRoot) == 13);
    p1.physicsRoot.objectID = -1;
    CHECK(physics_gGetRange(
              &p0.physicsRoot,
              &p1.physicsRoot) == -1);
    return 0;
}

int main(void)
{
    CHECK(test_geometry_stream_resolution() == 0);
    CHECK(test_public_state_conversion() == 0);
    CHECK(test_update_scene_object() == 0);
    CHECK(test_set_recoil() == 0);
    CHECK(test_driver_state_sync() == 0);
    CHECK(test_root_accessors_and_facing_lock() == 0);
    CHECK(test_pool_initialization() == 0);
    CHECK(test_pool_allocation_and_cleanup() == 0);
    CHECK(test_solid_builder_geometry_and_ownership() == 0);
    CHECK(test_facing_modification_publication() == 0);
    CHECK(test_turn_to_face() == 0);
    CHECK(test_turn_to_attack() == 0);
    CHECK(test_normal_movement_and_no_contact_commit() == 0);
    CHECK(test_move_player_direct_and_special_paths() == 0);
    CHECK(test_normal_movement_surface_forces() == 0);
    CHECK(test_blown_movement_state_machine() == 0);
    CHECK(test_hover_movement_state_machine() == 0);
    CHECK(test_flying_movement_state_machine() == 0);
    CHECK(test_coredeath_movement_state_machine() == 0);
    CHECK(test_solid_relative_world_transforms() == 0);
    CHECK(test_processed_standee_movement() == 0);
    CHECK(test_recursive_standee_cycle_scheduling() == 0);
    CHECK(test_contact_geometry_helpers() == 0);
    CHECK(test_plane_contact_helper() == 0);
    CHECK(test_sphere_polygon_contact_kernel() == 0);
    CHECK(test_general_solid_collision() == 0);
    CHECK(test_libpart_float_decode() == 0);
    CHECK(test_newclosest_poly_thin_map() == 0);
    CHECK(test_newclosest_poly_dynamic_solid() == 0);
    CHECK(test_world_blocking_core() == 0);
    CHECK(test_check_cube_blocking_no_contact() == 0);
    CHECK(test_check_cube_blocking_dynamic_contact() == 0);
    CHECK(test_map_event_tables() == 0);
    CHECK(test_launch_splash() == 0);
    CHECK(test_target_facing_helpers() == 0);
    CHECK(test_ground_target_and_object_owners() == 0);
    CHECK(test_nearest_physics_target() == 0);
    CHECK(test_snapshot_position() == 0);
    CHECK(test_character_blocking_kernel() == 0);
    CHECK(test_move_character_contact_sweep() == 0);
    CHECK(test_move_large_character_node_contacts() == 0);
    CHECK(test_range_cache_and_fallback() == 0);
    CHECK(test_launch_map_anim_effects() == 0);
    CHECK(test_process_physics_objects_scheduler() == 0);
    CHECK(test_reset_jedi_complete() == 0);
    CHECK(test_streets_ending_collision_trigger() == 0);
    puts("physics tests passed");
    return 0;
}
