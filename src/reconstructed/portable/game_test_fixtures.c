/* Shared host test fixtures (moved from the PC host so the Xbox boss smoke
   can place the player and activate bosses the same way). */
#include "jpb/game_test_fixtures.h"
#include "jpb/camera.h"
#include "jpb/enemy.h"
#include "jpb/globalarrays.h"
#include "jpb/intersec.h"
#include "jpb/jonny.h"
#include "jpb/physics.h"
#include <stdint.h>
#include <string.h>

static int test_collision_storage_contains(
    const JPBGameRuntime *runtime,
    const void *pointer,
    size_t size)
{
    uintptr_t start;
    uintptr_t address;

    if (runtime == NULL ||
        runtime->collisionStorage == NULL ||
        pointer == NULL ||
        size > runtime->collisionStorageSize) {
        return 0;
    }
    start = (uintptr_t)(const void *)runtime->collisionStorage;
    address = (uintptr_t)pointer;
    return address >= start &&
        address - start <= runtime->collisionStorageSize - size;
}

static int test_camera_index_from_height_stuff(
    const JPBGameRuntime *runtime,
    const _jheightstuff *height_stuff)
{
    const int32_t *camera_record;
    uint32_t cube_word;

    if (runtime == NULL || height_stuff == NULL || leveldata == NULL ||
        !test_collision_storage_contains(
            runtime,
            height_stuff->cube,
            sizeof(*height_stuff->cube))) {
        return -1;
    }
    camera_record = height_stuff->cube;
    cube_word = (uint32_t)*camera_record;
    if ((cube_word & UINT32_C(0x3c000000)) == 0) {
        if (!test_collision_storage_contains(
                runtime,
                leveldata - 4,
                5 * sizeof(*leveldata))) {
            return -1;
        }
        camera_record =
            leveldata +
            (leveldata[-4] >> 11) +
            (int32_t)(((cube_word >> 14) &
                       UINT32_C(0xff)) * 9U);
    }
    if (!test_collision_storage_contains(
            runtime, camera_record, 2 * sizeof(*camera_record))) {
        return -1;
    }
    return ((const uint8_t *)(const void *)camera_record)[7] &
        UINT8_C(0x7f);
}

int jpb_TestCameraIndexAtWorldPosition(
    const JPBGameRuntime *runtime,
    int32_t x,
    int32_t y,
    int32_t z)
{
    VECTOR high_point;
    _jheightstuff height_stuff;

    high_point.vx = x;
    high_point.vy = y + 0x100;
    high_point.vz = z;
    high_point.pad = 0;
    memset(&height_stuff, 0, sizeof(height_stuff));
    (void)intersec_FindWalkHeight(
        &high_point,
        NULL,
        (objectRoot *)(void *)&height_stuff,
        1);
    return test_camera_index_from_height_stuff(runtime, &height_stuff);
}

int jpb_TestPlacePlayer(
    JPBGameRuntime *runtime,
    const FVECTOR *position,
    int select_camera)
{
    if (runtime == NULL || runtime->physics == NULL ||
        runtime->world == NULL || position == NULL) {
        return 0;
    }
    runtime->physics->pos = *position;
    runtime->physics->vpos.vx = (int32_t)position->vx;
    runtime->physics->vpos.vy = (int32_t)position->vy;
    runtime->physics->vpos.vz = (int32_t)position->vz;
    runtime->physics->snapshotpos = runtime->physics->pos;
    runtime->physics->lastpos = runtime->physics->pos;
    runtime->physics->lastpolyhit = NULL;
    runtime->world->location.vx = runtime->physics->vpos.vx;
    runtime->world->location.vy = runtime->physics->vpos.vy;
    runtime->world->location.vz = runtime->physics->vpos.vz;
    runtime->targetX = runtime->physics->pos.vx;
    runtime->targetY = runtime->physics->pos.vy;
    runtime->targetZ = runtime->physics->pos.vz;
    if (select_camera) {
        int camera_index = jpb_TestCameraIndexAtWorldPosition(
            runtime,
            runtime->physics->vpos.vx,
            runtime->physics->vpos.vy,
            runtime->physics->vpos.vz);

        if (camera_index >= 0 && camera_index < 256) {
            runtime->world->currentDolly = (int16_t)camera_index;
            runtime->world->overRideDolly = 0;
            newcameraflag = 1;
        }
    }
    return jpb_PhysicsUpdateSceneObject(runtime->physics) ==
        JPB_PHYSICS_RESULT_OK;
}

int jpb_TestForceEnemyPlacement(
    JPBGameRuntime *runtime,
    int placement_index)
{
    wsl_BAP_PLACEMENT *placement;
    wsl_ENEMY *enemy;

    if (runtime == NULL ||
        runtime->world == NULL ||
        runtime->world->apEnemy == NULL ||
        runtime->player == NULL ||
        placement_index < 0 ||
        placement_index >= runtime->world->nEnemy) {
        return 0;
    }
    placement = runtime->world->apEnemy[placement_index];
    if (placement == NULL ||
        jpb_GameRuntimeEnemyClassModelId(
            runtime, placement->actorNum) < 0) {
        return 0;
    }
    if (placement->status == 0) {
        if (_addEnemy(
                placement,
                placement_index,
                -1,
                1) == 0) {
            return 0;
        }
        placement->status = 1;
    }
    if (placement->status != 1 ||
        placement->pLastEnemy == UINT32_MAX) {
        return 0;
    }
    enemy = (wsl_ENEMY *)getPtr(
        (int)placement->pLastEnemy,
        JPB_POINTER_ARRAY_ENEMY);
    if (enemy == NULL || enemy->pPlayer == NULL) {
        return 0;
    }
    runtime->player->target = enemy->pPlayer;
    runtime->enemy = enemy;
    return 1;
}
