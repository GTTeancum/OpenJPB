#include "jpb/bullet.h"

#include "jpb/alloc.h"
#include "jpb/ai.h"
#include "jpb/anim.h"
#include "jpb/bmd.h"
#include "jpb/boss.h"
#include "jpb/braindmg.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/effects.h"
#include "jpb/force.h"
#include "jpb/game.h"
#include "jpb/globalarrays.h"
#include "jpb/jedi.h"
#include "jpb/model.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/sprite.h"
#include "jpb/world.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                              \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int sound_calls;
static VECTOR *sound_position;
static int sound_bank;
static char sound_name[9];

static void clear_physics_player_pool(void)
{
    int index;

    memset(gaPlayerData, 0, sizeof(gaPlayerData));
    memset(maPhysicsData, 0, sizeof(maPhysicsData));
    for (index = 0; index < JPB_PLAYER_CAPACITY; ++index) {
        gaPlayerData[index].playerRoot.objectID = -1;
        maPhysicsData[index].physicsRoot.objectID = -1;
    }
}

static uint16_t capture_sound(
    void *chunk,
    int loops,
    VECTOR *position,
    int bank_id,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    (void)chunk;
    (void)loops;
    (void)flag;
    (void)user_data;
    ++sound_calls;
    sound_position = position;
    sound_bank = bank_id;
    memcpy(sound_name, sound, sizeof(sound_name));
    return 7;
}

static int test_terminate_sfx_string(void)
{
    char dest[10];

    memset(dest, 'x', sizeof(dest));
    TerminateSFXString(dest, "123456789");
    CHECK(memcmp(dest, "12345678", 8) == 0);
    CHECK(dest[8] == '\0');
    CHECK(dest[9] == 'x');
    return 0;
}

static int test_allocate_and_free(void)
{
    Projectile *first;
    Projectile *second;
    Projectile *reuse;
    Projectile zero;

    meminit();
    memset(&zero, 0, sizeof(zero));
    first = bullet_AllocProjectile(27);
    second = bullet_AllocProjectile(4);
    CHECK(first != NULL);
    CHECK(second != NULL);
    CHECK(first != second);
    CHECK(memsize(first) == 92);
    CHECK(first->pj_Type == 27);
    first->pj_Type = 0;
    CHECK(memcmp(first, &zero, sizeof(zero)) == 0);
    CHECK(second->pj_Type == 4);

    bullet_FreeProjectile(first);
    reuse = bullet_AllocProjectile(9);
    CHECK(reuse == first);
    CHECK(reuse->pj_Type == 9);
    bullet_FreeProjectile(reuse);
    bullet_FreeProjectile(second);
    return 0;
}

static int test_sound_name_initialization(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;

    memset(maProjTypes, 0xa5, sizeof(maProjTypes));
    bullet_InitProjectilePool();

    CHECK(memcmp(types[0].fireSound, "barnlasr", 8) == 0);
    CHECK(memcmp(types[1].fireSound, "jawalasr", 8) == 0);
    CHECK(memcmp(types[4].fireSound, "tanklasr", 8) == 0);
    CHECK(memcmp(types[10].fireSound, "\0\0\0\0\0\0\0\0", 8) == 0);
    CHECK(memcmp(types[17].fireSound, "riflfire", 8) == 0);
    CHECK(memcmp(types[27].fireSound, "dstfire1", 8) == 0);
    CHECK(memcmp(types[30].fireSound, "amidlasr", 8) == 0);

    CHECK(memcmp(types[5].hitSound, "pushjedi", 8) == 0);
    CHECK(memcmp(types[6].hitSound, "explomed", 8) == 0);
    CHECK(memcmp(types[8].hitSound, "explosm\0", 8) == 0);
    CHECK(memcmp(types[17].hitSound, "explomed", 8) == 0);
    CHECK(memcmp(types[24].hitSound, "pulsgrnd", 8) == 0);

    CHECK((uint8_t)types[0].range == UINT8_C(0xa5));
    CHECK((uint8_t)types[31].pad == UINT8_C(0xa5));
    return 0;
}

static int test_callback_character_collision(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject *owner;
    playerObject *target;
    sceneObject owner_scene;
    sceneObject target_scene;
    CollisionData collision = {20, 3, -1};
    Mnode hit_node;
    Motion motion;
    Motion *motion_ptr = &motion;
    _Material material;
    Projectile *proj;
    Sprite *parent;
    Sprite *child;
    VECTOR start = {100, 200, 300, 0};
    VECTOR end = {1100, 200, 300, 0};

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&owner_scene, 0, sizeof(owner_scene));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&hit_node, 0, sizeof(hit_node));
    memset(&motion, 0, sizeof(motion));
    memset(&material, 0, sizeof(material));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.CurrentLevel = 0;
    GameStruct.versusModeFlag = 0;
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    owner = &gaPlayerData[2];
    target = &gaPlayerData[3];
    owner->playerRoot.objectID = 2;
    owner->playerRoot.pParent = &owner_scene.sceneRoot;
    owner->playernum = 2;
    owner->playerID = 20;
    owner->target = target;
    owner->pMotion = &motion_ptr;
    owner_scene.pScene = &owner_scene.sceneRoot;
    owner_scene.pPlayer = &owner->playerRoot;
    owner_scene.pPhysics = &maPhysicsData[2].physicsRoot;
    target->playerRoot.objectID = 3;
    target->playerRoot.pParent = &target_scene.sceneRoot;
    target->playernum = 3;
    target->playerID = 20;
    target->paNodesSizes = &collision;
    target->numCollisionNodes = 1;
    target_scene.pScene = &target_scene.sceneRoot;
    target_scene.pPlayer = &target->playerRoot;
    target_scene.pPhysics = &maPhysicsData[3].physicsRoot;
    maPhysicsData[2].physicsRoot.objectID = 2;
    maPhysicsData[2].physicsRoot.pParent = &owner_scene.sceneRoot;
    maPhysicsData[3].physicsRoot.objectID = 3;
    maPhysicsData[3].physicsRoot.pParent = &target_scene.sceneRoot;
    maPhysicsData[3].vpos.vx = 116;
    maPhysicsData[3].vpos.vy = 200;
    maPhysicsData[3].vpos.vz = 300;
    hit_node.id = (modelNodeId)(NODE_DYNAMIC | 3);
    hit_node.v3RotCenter.vx = 116;
    hit_node.v3RotCenter.vy = 200;
    hit_node.v3RotCenter.vz = 300;
    coll_gRegisterNode(target->playernum, &hit_node);
    effects1Handle[3] = &material;
    types[2].range = 60;
    types[2].radius = 20;
    types[2].length = 16;
    types[2].width = 2;
    types[2].muzzelEffect = -1;
    types[2].bulletSprite = 3;
    types[2].speed = 32;
    types[2].flag = UINT16_C(4);
    bullet_InitProjectilePool();
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    proj = bullet_AllocProjectile(2);
    CHECK(proj != NULL);
    bullet_ShootProjectile(proj, owner, &start, &end, NULL);
    parent = proj->pj_Parent;
    child = proj->pj_Child;
    CHECK(parent != NULL);
    CHECK(child != NULL);
    CHECK(parent->sp_Func(
              (int32_t *)(void *)parent) == 1);
    CHECK(target->target == owner);
    CHECK(target->hitNumber == 1);
    CHECK(target->projectile == &types[2]);
    CHECK(target->whohitme == owner);
    CHECK(target->hitMotion == &motion);
    CHECK(target->hitLocation.vx == 116);
    CHECK(target->hitLocation.vy == 200);
    CHECK(target->hitLocation.vz == 300);
    CHECK((hit_node.flags & UINT32_C(0x20000)) != 0);
    CHECK((parent->sp_Flags & UINT32_C(1)) != 0);
    CHECK((child->sp_Flags & UINT32_C(1)) != 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    clear_physics_player_pool();
    return 0;
}

static int test_projectile_explosion(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject *target;
    sceneObject scene;
    VECTOR center = {100, 200, 300, 0};
    uint32_t mask = UINT32_C(3);

    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&scene, 0, sizeof(scene));
    target = &gaPlayerData[4];
    target->playerRoot.objectID = 4;
    target->playerRoot.pParent = &scene.sceneRoot;
    target->hitNumber = UINT8_C(7);
    scene.pScene = &scene.sceneRoot;
    scene.pPlayer = &target->playerRoot;
    scene.pPhysics = &maPhysicsData[4].physicsRoot;
    maPhysicsData[4].physicsRoot.objectID = 4;
    maPhysicsData[4].physicsRoot.pParent = &scene.sceneRoot;
    maPhysicsData[4].vpos = center;

    bullet_Explosion(&center, &mask, 0x200, 6);
    CHECK(target->hitNumber == UINT8_C(8));
    CHECK(target->projectile == &types[6]);
    CHECK((mask & (UINT32_C(1) << 4)) != 0);

    bullet_Explosion(&center, &mask, 0x200, 7);
    CHECK(target->hitNumber == UINT8_C(8));
    CHECK(target->projectile == &types[6]);
    clear_physics_player_pool();
    return 0;
}

static int test_callback_world_bounce(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    physicsObject old_physics[JPB_PHYSICS_CAPACITY];
    geomData geometry;
    _solid solid;
    _svector vertices[4] = {
        {-10, 0, -10, 0},
        {10, 0, -10, 0},
        {10, 0, 10, 0},
        {-10, 0, 10, 0}
    };
    _svector normals[1] = {{0, 4096, 0, 0}};
    int16_t indices[4] = {0, 1, 2, 3};
    Projectile projectile;
    Sprite callback;
    playerObject owner;
    float old_frame_rate = fGlobalFrameRate;
    int old_fixed_frame_rate = gGlobalFrameRate;
    int old_level = (int)(int8_t)LevelSelect;
    int i;

    memcpy(old_physics, maPhysicsData, sizeof(old_physics));
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&geometry, 0, sizeof(geometry));
    memset(&solid, 0, sizeof(solid));
    memset(&projectile, 0, sizeof(projectile));
    memset(&callback, 0, sizeof(callback));
    memset(&owner, 0, sizeof(owner));
    for (i = 0; i < JPB_PHYSICS_CAPACITY; ++i) {
        memset(&maPhysicsData[i], 0, sizeof(maPhysicsData[i]));
        maPhysicsData[i].physicsRoot.objectID = -1;
    }
    pointerRegistry_Reset();
    geometry.numFaces = 1;
    geometry.pIndex = addPtr(indices, JPB_POINTER_ARRAY_INDEX);
    CHECK(geometry.pIndex >= 0);
    solid.geometry = &geometry;
    solid.coords = vertices;
    solid.normals = normals;
    maPhysicsData[5].physicsRoot.objectID = 5;
    maPhysicsData[5].solid = &solid;

    types[2].flag = UINT16_C(0x80);
    types[2].hitEffect = -1;
    projectile.pj_Start.vy = 10;
    projectile.pj_Dir.vy = -4096;
    owner.playerRoot.objectID = 2;
    projectile.pj_Dir.speed = 160;
    projectile.pj_Range = 5;
    projectile.pj_Type = 2;
    projectile.pj_Owner = (int32_t *)(void *)&owner;
    callback.sp_User = (int32_t *)(void *)&projectile;
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    fGlobalFrameRate = 1.0f;
    gGlobalFrameRate = 4096;
    LevelSelect = 1;

    CHECK(bullet_CallBack((Projectile *)(void *)&callback) == 0);
    CHECK(projectile.pj_Start.vx == 0);
    CHECK(projectile.pj_Start.vy == 0);
    CHECK(projectile.pj_Start.vz == 0);
    CHECK(projectile.pj_Dir.vx == 0);
    CHECK(projectile.pj_Dir.vy == 4096);
    CHECK(projectile.pj_Dir.vz == 0);
    CHECK(projectile.pj_Dir.speed == 136);
    CHECK(projectile.pj_Range == 4);

    LevelSelect = (char)old_level;
    fGlobalFrameRate = old_frame_rate;
    gGlobalFrameRate = old_fixed_frame_rate;
    memcpy(maPhysicsData, old_physics, sizeof(old_physics));
    pointerRegistry_Reset();
    return 0;
}

static int test_callback_ballistic_and_homing(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    Projectile projectile;
    Sprite callback;
    playerObject owner;
    playerObject target;
    Mnode pelvis;
    float old_frame_rate = fGlobalFrameRate;
    int old_fixed_frame_rate = gGlobalFrameRate;

    clear_physics_player_pool();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&projectile, 0, sizeof(projectile));
    memset(&callback, 0, sizeof(callback));
    memset(&owner, 0, sizeof(owner));
    memset(&target, 0, sizeof(target));
    memset(&pelvis, 0, sizeof(pelvis));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.versusModeFlag = 0;
    fGlobalFrameRate = 1.0f;
    gGlobalFrameRate = 4096;
    owner.playerRoot.objectID = 2;
    owner.playernum = 2;
    target.playernum = 3;
    projectile.pj_Start.vy = 1000;
    projectile.pj_Dir.vz = 4096;
    projectile.pj_Dir.speed = 80;
    projectile.pj_Range = 5;
    projectile.pj_Type = 2;
    projectile.pj_Owner = (int32_t *)(void *)&owner;
    callback.sp_User = (int32_t *)(void *)&projectile;
    types[2].flag = UINT16_C(2 | 8);
    types[2].rangeEffect = -1;
    types[2].hitEffect = -1;
    types[2].bulletEffect = -1;

    CHECK(bullet_CallBack((Projectile *)(void *)&callback) == 0);
    CHECK(projectile.pj_Dir.vy < 0);
    CHECK(projectile.pj_Dir.vz > 0);
    CHECK(projectile.pj_Dir.speed >= 80);
    CHECK(projectile.pj_Range == 4);

    memset(&projectile, 0, sizeof(projectile));
    memset(&callback, 0, sizeof(callback));
    pelvis.id = (modelNodeId)(NODE_DYNAMIC | 0);
    pelvis.v3RotCenter.vx = 1000;
    pelvis.v3RotCenter.vy = 1000;
    pelvis.v3RotCenter.vz = 0;
    coll_gRegisterNode(target.playernum, &pelvis);
    projectile.pj_Start.vy = 1000;
    projectile.pj_Dir.vz = 4096;
    projectile.pj_Dir.speed = 80;
    projectile.pj_Range = 5;
    projectile.pj_Type = 2;
    projectile.pj_Owner = (int32_t *)(void *)&owner;
    projectile.pj_Target = (int32_t *)(void *)&target;
    callback.sp_User = (int32_t *)(void *)&projectile;
    types[2].flag = UINT16_C(0x100 | 8);

    CHECK(bullet_CallBack((Projectile *)(void *)&callback) == 0);
    CHECK(projectile.pj_Dir.vx > 0);
    CHECK(projectile.pj_Dir.vz > 0);
    CHECK(projectile.pj_Dir.speed == 80);
    CHECK(projectile.pj_Range == 4);

    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    fGlobalFrameRate = old_frame_rate;
    gGlobalFrameRate = old_fixed_frame_rate;
    return 0;
}

static int test_callback_termination_audio(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject owner;
    Projectile *projectile;
    Sprite callback;
    float old_frame_rate = fGlobalFrameRate;
    int old_fixed_frame_rate = gGlobalFrameRate;

    meminit();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&owner, 0, sizeof(owner));
    memset(&callback, 0, sizeof(callback));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    fGlobalFrameRate = 1.0f;
    gGlobalFrameRate = 4096;
    owner.playerRoot.objectID = 2;
    types[2].flag = UINT16_C(0x40);
    types[2].rangeEffect = -1;
    types[2].hitEffect = -1;
    types[2].bulletEffect = -1;
    memcpy(types[2].hitSound, "explomed", 8);
    projectile = bullet_AllocProjectile(2);
    CHECK(projectile != NULL);
    projectile->pj_Range = 1;
    projectile->pj_Owner = (int32_t *)(void *)&owner;
    callback.sp_User = (int32_t *)(void *)projectile;
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    CHECK(bullet_CallBack((Projectile *)(void *)&callback) == 1);
    CHECK(sound_calls == 1);
    CHECK(sound_bank == 0);
    CHECK(strcmp(sound_name, "explomed") == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    clear_physics_player_pool();
    fGlobalFrameRate = old_frame_rate;
    gGlobalFrameRate = old_fixed_frame_rate;
    return 0;
}

static int test_clear_projectiles(void)
{
    Projectile pool[3];
    Projectile zero[3];

    memset(pool, 0x5a, sizeof(pool));
    memset(zero, 0, sizeof(zero));
    sprite_ClearProjectilePool(pool, 0);
    CHECK((unsigned char)pool[0].pad[0] == 0x5a);
    sprite_ClearProjectilePool(pool, 3);
    CHECK(memcmp(pool, zero, sizeof(pool)) == 0);
    return 0;
}

static int test_sprite_backed_projectile(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    Projectile *proj;
    _Material material;
    VECTOR center = {100, 200, 300, 0};
    _svector rotation = {0, 0, 0, 0};
    Sprite *parent;
    Sprite *child;

    meminit();
    sprite_gInitSprites();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&material, 0, sizeof(material));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    effects1Handle[3] = &material;
    types[2].length = 10;
    types[2].width = 2;
    types[2].bulletSprite = 3;

    proj = bullet_AllocProjectile(2);
    CHECK(proj != NULL);
    parent = sprite_Get3DProjectile(proj, &center, &rotation);
    CHECK(parent != NULL);
    child = proj->pj_Child;
    CHECK(proj->pj_Parent == parent);
    CHECK(child != NULL);
    CHECK(parent->sp_SCB->scb_Texture == &material);
    CHECK(child->sp_SCB->scb_Texture == &material);
    CHECK(parent->sp_SCB->scb_flags == 4);
    CHECK(child->sp_SCB->scb_flags == 4);
    CHECK(parent->sp_SCB->scb_vertex0.vx == 100.0f);
    CHECK(parent->sp_SCB->scb_vertex0.vy == 202.0f);
    CHECK(parent->sp_SCB->scb_vertex0.vz == 300.0f);
    CHECK(parent->sp_SCB->scb_vertex1.vz == 290.0f);
    CHECK(parent->sp_SCB->scb_vertex2.vy == 198.0f);
    CHECK(rotation.vz == 0x400);
    CHECK(child->sp_Rot.vz == 0x400);
    CHECK(child->sp_SCB->scb_vertex0.vx == 98.0f);
    CHECK(child->sp_SCB->scb_vertex0.vy == 200.0f);
    CHECK(child->sp_SCB->scb_vertex0.vz == 300.0f);
    CHECK(child->sp_SCB->scb_vertex1.vx == 98.0f);
    CHECK(child->sp_SCB->scb_vertex1.vy == 200.0f);
    CHECK(child->sp_SCB->scb_vertex1.vz == 290.0f);
    CHECK(child->sp_SCB->scb_vertex2.vx == 102.0f);
    CHECK(child->sp_SCB->scb_vertex2.vy == 200.0f);
    CHECK(child->sp_SCB->scb_vertex2.vz == 300.0f);
    CHECK(child->sp_SCB->scb_vertex3.vx == 102.0f);
    CHECK(child->sp_SCB->scb_vertex3.vy == 200.0f);
    CHECK(child->sp_SCB->scb_vertex3.vz == 290.0f);

    sprite_RemoveProjectile(proj);
    CHECK((parent->sp_Flags & 1) != 0);
    CHECK((child->sp_Flags & 1) != 0);
    CHECK((parent->sp_SCB->scb_flags & 1) != 0);
    CHECK((child->sp_SCB->scb_flags & 1) != 0);
    return 0;
}

static int test_shoot_projectile(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject player;
    playerObject target;
    Motion motion;
    Motion *motion_ptr = &motion;
    Projectile *proj;
    _Material material;
    VECTOR start = {100, 200, 300, 0};
    VECTOR end = {100, 200, 1300, 0};
    float parent_z;
    int callback_result;

    meminit();
    sprite_gInitSprites();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&player, 0, sizeof(player));
    memset(&target, 0, sizeof(target));
    memset(&motion, 0, sizeof(motion));
    memset(&material, 0, sizeof(material));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    OptionStruct.ShockFlag[0] = 0;
    player.playernum = 0;
    player.target = &target;
    player.pMotion = &motion_ptr;
    effects1Handle[3] = &material;
    types[2].range = 60;
    types[2].length = 16;
    types[2].width = 2;
    types[2].muzzelEffect = -1;
    types[2].bulletSprite = 3;
    types[2].speed = 32;
    bullet_InitProjectilePool();
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    proj = bullet_AllocProjectile(2);
    CHECK(proj != NULL);
    bullet_ShootProjectile(proj, &player, &start, &end, NULL);
    CHECK(proj->pj_Start.vx == 100);
    CHECK(proj->pj_Start.vy == 200);
    CHECK(proj->pj_Start.vz == 300);
    CHECK(proj->pj_Dir.vx == 0);
    CHECK(proj->pj_Dir.vy == 0);
    CHECK(proj->pj_Dir.vz == 4096);
    CHECK(proj->pj_Dir.speed == 256);
    CHECK(proj->pj_Range == 120);
    CHECK(proj->pj_Owner == (int32_t *)(void *)&player);
    CHECK(proj->pj_Target == (int32_t *)(void *)&target);
    CHECK(proj->pj_User == (int32_t *)(void *)&motion);
    CHECK(proj->pj_Parent != NULL);
    CHECK(proj->pj_Child != NULL);
    CHECK(proj->pj_Parent->sp_User == (int32_t *)(void *)proj);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &start);
    CHECK(sound_bank == 1);
    CHECK(strcmp(sound_name, "tankfire") == 0);

    parent_z = proj->pj_Parent->sp_SCB->scb_vertex0.vz;
    callback_result = proj->pj_Parent->sp_Func(
        (int32_t *)(void *)proj->pj_Parent);
    CHECK(callback_result == 0);
    CHECK(proj->pj_Start.vz == 316);
    CHECK(proj->pj_Range == 119);
    CHECK(proj->pj_Parent->sp_SCB->scb_vertex0.vz == parent_z + 16.0f);
    sprite_RemoveProjectile(proj);
    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

static int test_starfighter_twin_shot(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject player;
    playerObject target;
    Motion motion;
    Motion *motion_ptr = &motion;
    sceneObject scene;
    physicsObject physics;
    Mnode left;
    Mnode right;
    Mnode target_node;
    _Material material;

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&player, 0, sizeof(player));
    memset(&target, 0, sizeof(target));
    memset(&motion, 0, sizeof(motion));
    memset(&scene, 0, sizeof(scene));
    memset(&physics, 0, sizeof(physics));
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    memset(&target_node, 0, sizeof(target_node));
    memset(&material, 0, sizeof(material));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    OptionStruct.ShockFlag[0] = 0;
    player.playernum = 0;
    player.target = &target;
    player.pMotion = &motion_ptr;
    player.playerRoot.pParent = (objectRoot *)(void *)&scene;
    scene.pPhysics = (objectRoot *)(void *)&physics;
    physics.vpos.vx = 7.0f;
    physics.vpos.vy = 8.0f;
    physics.vpos.vz = 9.0f;
    target.playernum = 1;
    motion.fx2 = 2;
    left.id = (modelNodeId)(NODE_DYNAMIC | 13);
    left.v3RotCenter.vx = 100;
    left.v3RotCenter.vy = 200;
    left.v3RotCenter.vz = 300;
    right.id = (modelNodeId)(NODE_DYNAMIC | 8);
    right.v3RotCenter.vx = -100;
    right.v3RotCenter.vy = 200;
    right.v3RotCenter.vz = 300;
    target_node.id = (modelNodeId)(NODE_DYNAMIC | 0);
    target_node.v3RotCenter.vx = 1000;
    target_node.v3RotCenter.vy = 200;
    target_node.v3RotCenter.vz = 1300;
    coll_gRegisterNode(0, &left);
    coll_gRegisterNode(0, &right);
    coll_gRegisterNode(1, &target_node);
    effects1Handle[3] = &material;
    types[2].range = 60;
    types[2].length = 16;
    types[2].width = 2;
    types[2].muzzelEffect = -1;
    types[2].bulletSprite = 3;
    types[2].speed = 32;
    bullet_InitProjectilePool();
    sound_calls = 0;
    sound_FreeBank(0);
    sound_FreeBank(3);
    CHECK(sound_LoadBank("fed", 0) == 0);
    CHECK(sound_LoadBank("theed", 3) == 0);
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    srand(1);
    boss_StarFighterBlaster(&player, 0);
    CHECK(sound_calls == 4);
    CHECK(strcmp(sound_name, "tankfire") == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    sound_FreeBank(0);
    sound_FreeBank(3);
    CHECK(sound_LoadBank("resident", 0) == 0);
    CHECK(sound_LoadBank("resident", 3) == 0);
    return 0;
}

static int test_projectile_reflection(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    CollisionData collision = {10, 3, -1};
    playerObject owner;
    playerObject target;
    sceneObject owner_scene;
    sceneObject target_scene;
    physicsObject owner_physics;
    physicsObject target_physics;
    Motion target_motion;
    Motion *target_motion_ptr = &target_motion;
    Mnode hit_node;
    Mnode saber_node;
    _Material material;
    Projectile *proj;

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(damageTracking, 0, sizeof(damageTracking));
    memset(&owner, 0, sizeof(owner));
    memset(&target, 0, sizeof(target));
    memset(&owner_scene, 0, sizeof(owner_scene));
    memset(&target_scene, 0, sizeof(target_scene));
    memset(&owner_physics, 0, sizeof(owner_physics));
    memset(&target_physics, 0, sizeof(target_physics));
    memset(&target_motion, 0, sizeof(target_motion));
    memset(&hit_node, 0, sizeof(hit_node));
    memset(&saber_node, 0, sizeof(saber_node));
    memset(&material, 0, sizeof(material));
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.CurrentLevel = 0;
    fGlobalFrameRate = 0.5f;
    gGlobalFrameRate = 2048;
    OptionStruct.FunFactor &= UINT8_C(0xfe);
    owner.playernum = 2;
    owner.playerID = 20;
    owner.playerRoot.pParent = &owner_scene.sceneRoot;
    owner_scene.pPlayer = &owner.playerRoot;
    owner_scene.pPhysics = &owner_physics.physicsRoot;
    target.playernum = 1;
    target.playerID = 0;
    target.playerRoot.objectID = 0;
    target.playerRoot.pParent = &target_scene.sceneRoot;
    target.pMotion = &target_motion_ptr;
    target_scene.pPlayer = &target.playerRoot;
    target_scene.pPhysics = &target_physics.physicsRoot;
    target.paNodesSizes = &collision;
    target.numCollisionNodes = 1;
    hit_node.id = (modelNodeId)(NODE_DYNAMIC | 3);
    hit_node.v3RotCenter.vx = 100;
    hit_node.v3RotCenter.vy = 200;
    hit_node.v3RotCenter.vz = 300;
    saber_node.id = (modelNodeId)(NODE_DYNAMIC | 12);
    saber_node.flags = UINT32_C(1);
    coll_gRegisterNode(target.playernum, &hit_node);
    coll_gRegisterNode(target.playernum, &saber_node);
    effects1Handle[3] = &material;
    types[2].range = 60;
    types[2].radius = 10;
    types[2].length = 16;
    types[2].width = 2;
    types[2].muzzelEffect = -1;
    types[2].bulletSprite = 3;
    types[2].speed = 32;
    bullet_InitProjectilePool();
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    proj = bullet_AllocProjectile(2);
    CHECK(proj != NULL);
    proj->pj_Start = hit_node.v3RotCenter;
    proj->pj_Dir.speed = 64;
    proj->pj_Range = 30;
    proj->pj_Owner = (int32_t *)(void *)&owner;
    proj->pj_Target = (int32_t *)(void *)&target;
    srand(1);
    CHECK(coll_CheckProjectileCollision(proj) == -1);
    CHECK((target.pFlags & UINT32_C(0x20)) != 0);
    CHECK(target.target == &owner);
    CHECK(proj->pj_Owner == (int32_t *)(void *)&target);
    CHECK(proj->pj_Target == (int32_t *)(void *)&owner);
    CHECK(proj->pj_Start.vx == 100);
    CHECK(proj->pj_Start.vy == 200);
    CHECK(proj->pj_Start.vz == 300);
    CHECK(proj->pj_Dir.speed == 512);
    CHECK(proj->pj_Range == 60);
    CHECK(proj->pj_Parent != NULL);
    CHECK(proj->pj_Child != NULL);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &target_physics.vpos);
    CHECK(sound_bank == 0);
    CHECK(strcmp(sound_name, "rico1") == 0);

    sprite_RemoveProjectile(proj);
    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

static void configure_callback_projectile_type(
    ProjType *type,
    int bullet_sprite,
    const char *sound)
{
    type->range = 60;
    type->radius = 10;
    type->length = 16;
    type->width = 2;
    type->muzzelEffect = -1;
    type->bulletSprite = (int8_t)bullet_sprite;
    type->speed = 32;
    type->flag = UINT16_C(4);
    memset(type->fireSound, 0, sizeof(type->fireSound));
    memcpy(type->fireSound, sound, strlen(sound));
}

static int test_jedi_weapon_callbacks(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject player;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    animObject animation;
    animListNode sequence;
    _animTemplate template_data;
    Motion motion;
    Motion *motion_ptr = &motion;
    Mnode muzzle;
    Mnode aim;
    Mnode aim2;
    _Material material;

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&animation, 0, sizeof(animation));
    memset(&sequence, 0, sizeof(sequence));
    memset(&template_data, 0, sizeof(template_data));
    memset(&motion, 0, sizeof(motion));
    memset(&muzzle, 0, sizeof(muzzle));
    memset(&aim, 0, sizeof(aim));
    memset(&aim2, 0, sizeof(aim2));
    memset(&material, 0, sizeof(material));
    bullet_InitProjectilePool();

    player.playerRoot.objectID = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playernum = 0;
    player.playerID = 6;
    player.currentMotion = 0x11;
    player.pMotion = &motion_ptr;
    motion.fx2 = 2;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    scene.pAnim = &animation.animRoot;
    physics.physicsRoot.pParent = &scene.sceneRoot;
    model.eventMask = 1;
    GameStruct.GameState &= ~UINT32_C(0x02000000);
    GameStruct.versusModeFlag = 0;
    gGlobalTimer = 1;
    game_gSetPowerLevel(0, 0);
    mDrawingSurfaceId = 0;

    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 0x0c);
    muzzle.v3RotCenter.vx = 100;
    muzzle.v3RotCenter.vy = 200;
    muzzle.v3RotCenter.vz = 300;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x11);
    aim.v3RotCenter.vx = 900;
    aim.v3RotCenter.vy = 250;
    aim.v3RotCenter.vz = 300;
    coll_gRegisterNode(player.playernum, &muzzle);
    coll_gRegisterNode(player.playernum, &aim);
    effects1Handle[3] = &material;
    configure_callback_projectile_type(
        &types[2], 3, "jedihit");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);

    CHECK(jedi_FireWeapon(NULL, &player) == 0);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &muzzle.v3RotCenter);
    CHECK(strcmp(sound_name, "jedihit") == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&muzzle, 0, sizeof(muzzle));
    memset(&aim, 0, sizeof(aim));
    bullet_InitProjectilePool();
    player.playerID = 0x15;
    player.currentMotion = 0x15;
    animation.pCurrentAnimSeq = &sequence;
    sequence.pAnimTemplate = &template_data;
    animation.animFrameIndex = JPB_FIXED_ONE;
    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 0x0e);
    muzzle.v3RotCenter.vx = 400;
    muzzle.v3RotCenter.vy = 500;
    muzzle.v3RotCenter.vz = 600;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x12);
    aim.v3RotCenter.vx = 1000;
    aim.v3RotCenter.vy = 500;
    aim.v3RotCenter.vz = 600;
    coll_gRegisterNode(player.playernum, &muzzle);
    coll_gRegisterNode(player.playernum, &aim);
    configure_callback_projectile_type(
        &types[10], 3, "sabrhit1");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    srand(1);

    CHECK(jedi_FireWeapon(NULL, &player) == 0);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &muzzle.v3RotCenter);
    CHECK(strcmp(sound_name, "sabrhit1") == 0);

    coll_ResetCollisionSystem();
    memset(&muzzle, 0, sizeof(muzzle));
    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 12);
    coll_gRegisterNode(player.playernum, &muzzle);
    template_data.Fframe = 5;
    animation.animFrameIndex = 15 * JPB_FIXED_ONE;
    CHECK(tusken_stab(NULL, &player) == 0);
    CHECK((muzzle.flags & UINT32_C(0x40000000)) != 0);
    animation.animFrameIndex = 17 * JPB_FIXED_ONE;
    CHECK(tusken_stab(NULL, &player) == 1);
    CHECK((muzzle.flags & UINT32_C(0x40000000)) == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&muzzle, 0, sizeof(muzzle));
    memset(&aim, 0, sizeof(aim));
    bullet_InitProjectilePool();
    player.playerID = 0;
    player.currentMotion = 1;
    template_data.Fframe = 0;
    animation.animFrameIndex = 8 * JPB_FIXED_ONE;
    GameStruct.aCharacterData[0].MaxForce = 255;
    game_gSetForce(0, 100);
    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 8);
    muzzle.v3RotCenter.vx = 120;
    muzzle.v3RotCenter.vy = 220;
    muzzle.v3RotCenter.vz = 320;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x0f);
    aim.v3RotCenter.vx = 920;
    aim.v3RotCenter.vy = 420;
    aim.v3RotCenter.vz = 520;
    coll_gRegisterNode(player.playernum, &muzzle);
    coll_gRegisterNode(player.playernum, &aim);
    configure_callback_projectile_type(
        &types[16], 3, "sabrsw01");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_PushCallBack(NULL, &player) == 1);
    CHECK(game_gGetForce(0) == 80);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &muzzle.v3RotCenter);
    CHECK(strcmp(sound_name, "sabrsw01") == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&muzzle, 0, sizeof(muzzle));
    memset(&aim, 0, sizeof(aim));
    memset(&aim2, 0, sizeof(aim2));
    bullet_InitProjectilePool();
    animation.animFrameIndex = 2 * JPB_FIXED_ONE;
    game_gSetForce(0, 100);
    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 8);
    muzzle.v3RotCenter.vx = 140;
    muzzle.v3RotCenter.vy = 240;
    muzzle.v3RotCenter.vz = 340;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x0f);
    aim.v3RotCenter.vx = 940;
    aim.v3RotCenter.vy = 440;
    aim.v3RotCenter.vz = 540;
    aim2.id = (modelNodeId)(NODE_DYNAMIC | 0x0b);
    aim2.v3RotCenter.vx = 840;
    aim2.v3RotCenter.vy = 440;
    aim2.v3RotCenter.vz = 440;
    coll_gRegisterNode(player.playernum, &muzzle);
    coll_gRegisterNode(player.playernum, &aim);
    coll_gRegisterNode(player.playernum, &aim2);
    configure_callback_projectile_type(
        &types[28], 3, "rico2");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_Ranged3CallBack(NULL, &player) == 1);
    CHECK(game_gGetForce(0) == 60);
    CHECK(sound_calls == 2);
    CHECK(sound_position == &muzzle.v3RotCenter);
    CHECK(strcmp(sound_name, "rico2") == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    bullet_InitProjectilePool();
    player.playerID = 1;
    animation.animFrameIndex = 2 * JPB_FIXED_ONE;
    game_gSetForce(0, 100);
    physics.vpos.vx = 160;
    physics.vpos.vy = 260;
    physics.vpos.vz = 360;
    configure_callback_projectile_type(
        &types[11], 3, "rico4");
    LevelSelect = 9;
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_RingCallBack(NULL, &player) == 1);
    CHECK(game_gGetForce(0) == 85);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &physics.vpos);
    CHECK(strcmp(sound_name, "rico4") == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&aim, 0, sizeof(aim));
    bullet_InitProjectilePool();
    player.playerID = 6;
    animation.animFrameIndex = 5 * JPB_FIXED_ONE;
    GameStruct.aCharacterData[0].Energy = 10;
    GameStruct.aCharacterData[0].Items = 2;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x0f);
    aim.v3RotCenter.vx = 180;
    aim.v3RotCenter.vy = 280;
    aim.v3RotCenter.vz = 380;
    coll_gRegisterNode(player.playernum, &aim);
    configure_callback_projectile_type(
        &types[6], 3, "bodyfal1");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_TossCallBack(NULL, &player) == 1);
    CHECK(GameStruct.aCharacterData[0].Items == 1);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &aim.v3RotCenter);
    CHECK(strcmp(sound_name, "bodyfal1") == 0);

    GameStruct.aCharacterData[0].Energy = 0;
    physics.vpos.vx = 190;
    physics.vpos.vy = 290;
    physics.vpos.vz = 390;
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_TossCallBack(NULL, &player) == 1);
    CHECK(GameStruct.aCharacterData[0].Items == 1);
    CHECK(sound_calls == 0);

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&aim, 0, sizeof(aim));
    bullet_InitProjectilePool();
    player.playerID = 0x22;
    animation.animFrameIndex = 5 * JPB_FIXED_ONE;
    aim.id = (modelNodeId)(NODE_DYNAMIC | 0x0f);
    aim.v3RotCenter.vx = 200;
    aim.v3RotCenter.vy = 300;
    aim.v3RotCenter.vz = 400;
    coll_gRegisterNode(player.playernum, &aim);
    configure_callback_projectile_type(
        &types[23], 3, "grenade");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));

    CHECK(force_TossGrenadeCallBack(
              NULL, &player) == 1);
    CHECK(GameStruct.aCharacterData[0].Items == 1);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &aim.v3RotCenter);
    CHECK(strcmp(sound_name, "grenade") == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

static int test_ai_fire_weapon_callback(void)
{
    ProjType *types = (ProjType *)(void *)maProjTypes;
    playerObject player;
    playerObject target;
    sceneObject scene;
    modelObject model;
    physicsObject physics;
    Motion motion;
    Motion *motion_ptr = &motion;
    Mnode muzzle;
    Mnode target_body;
    _Material material;

    meminit();
    sprite_gInitSprites();
    coll_ResetCollisionSystem();
    clear_physics_player_pool();
    memset(maProjTypes, 0, sizeof(maProjTypes));
    memset(&player, 0, sizeof(player));
    memset(&target, 0, sizeof(target));
    memset(&scene, 0, sizeof(scene));
    memset(&model, 0, sizeof(model));
    memset(&physics, 0, sizeof(physics));
    memset(&motion, 0, sizeof(motion));
    memset(&muzzle, 0, sizeof(muzzle));
    memset(&target_body, 0, sizeof(target_body));
    memset(&material, 0, sizeof(material));
    bullet_InitProjectilePool();

    player.playerRoot.objectID = 0;
    player.playerRoot.pParent = &scene.sceneRoot;
    player.playernum = 0;
    player.playerID = 6;
    player.target = &target;
    player.pMotion = &motion_ptr;
    target.playerRoot.objectID = 2;
    target.playernum = 2;
    scene.pModel = &model.modelRoot;
    scene.pPhysics = &physics.physicsRoot;
    motion.fx2 = 0x13;
    model.eventMask = UINT32_C(1);
    model.flags = UINT32_C(4);

    muzzle.id = (modelNodeId)(NODE_DYNAMIC | 0);
    muzzle.v3RotCenter.vx = 250;
    muzzle.v3RotCenter.vy = 350;
    muzzle.v3RotCenter.vz = 450;
    target_body.id = (modelNodeId)(NODE_DYNAMIC | 8);
    target_body.v3RotCenter.vx = 850;
    target_body.v3RotCenter.vy = 350;
    target_body.v3RotCenter.vz = 450;
    coll_gRegisterNode(0, &muzzle);
    coll_gRegisterNode(2, &target_body);
    effects1Handle[3] = &material;
    configure_callback_projectile_type(
        &types[0x13], 3, "explomed");
    sound_calls = 0;
    sound_position = NULL;
    memset(sound_name, 0, sizeof(sound_name));
    jpb_SoundSetPlaySfxHook(capture_sound, NULL);
    srand(1);

    CHECK(ai_FireWeapon(NULL, &player) == 1);
    CHECK(sound_calls == 1);
    CHECK(sound_position == &muzzle.v3RotCenter);
    CHECK(strcmp(sound_name, "explomed") == 0);

    model.flags = 0;
    model.eventMask = 0;
    sound_calls = 0;
    CHECK(ai_FireWeapon(NULL, &player) == 0);
    CHECK(sound_calls == 0);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

int main(void)
{
    if (sound_LoadBank("resident", 0) != 0) return 1;
    if (sound_LoadBank("theed", 1) != 0) return 1;
    if (sound_LoadBank("resident", 2) != 0) return 1;
    if (sound_LoadBank("resident", 3) != 0) return 1;
    if (test_terminate_sfx_string() != 0) return 1;
    if (test_allocate_and_free() != 0) return 1;
    if (test_sound_name_initialization() != 0) return 1;
    if (test_clear_projectiles() != 0) return 1;
    if (test_sprite_backed_projectile() != 0) return 1;
    if (test_shoot_projectile() != 0) return 1;
    if (test_callback_character_collision() != 0) return 1;
    if (test_callback_world_bounce() != 0) return 1;
    if (test_callback_ballistic_and_homing() != 0) return 1;
    if (test_callback_termination_audio() != 0) return 1;
    if (test_projectile_explosion() != 0) return 1;
    if (test_starfighter_twin_shot() != 0) return 1;
    if (test_projectile_reflection() != 0) return 1;
    if (test_jedi_weapon_callbacks() != 0) return 1;
    if (test_ai_fire_weapon_callback() != 0) return 1;
    sound_FreeBank(0);
    sound_FreeBank(1);
    sound_FreeBank(2);
    sound_FreeBank(3);
    puts("bullet tests passed");
    return 0;
}
