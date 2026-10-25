#ifndef JPB_GAME_TEST_FIXTURES_H
#define JPB_GAME_TEST_FIXTURES_H
/* Process-local test fixtures shared by the PC and Xbox hosts: place the
   player at an authored point and activate an enemy placement, as the boss
   smoke tests do. They never run in normal play. */
#include "jpb/game_runtime.h"
#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Authored camera dolly for the walkable surface under a world point, or
   -1 when the collision data cannot answer. */
int jpb_TestCameraIndexAtWorldPosition(const JPBGameRuntime *runtime,
    int32_t x, int32_t y, int32_t z);
/* Moves the player to position. With select_camera, also switches to the
   authored camera for that point. Returns 0 if the scene object could not
   be updated. */
int jpb_TestPlacePlayer(JPBGameRuntime *runtime, const FVECTOR *position,
    int select_camera);
/* Activates world enemy placement index (spawning it if dormant) and makes
   it the player's target. Returns 0 when the placement cannot be used. */
int jpb_TestForceEnemyPlacement(JPBGameRuntime *runtime, int placement_index);

#ifdef __cplusplus
}
#endif
#endif
