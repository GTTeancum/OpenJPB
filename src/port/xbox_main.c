#include "jpb/alloc.h"
#include "jpb/fmath.h"
#include "jpb/game.h"
#include "jpb/list.h"
#include "jpb/memory.h"
#include "jpb/sprite.h"
#include "jpb/timer.h"
#include "jpb/vectors.h"
#include "jpb/world.h"

#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>

/*
 * Milestone 1 Xbox bootstrap.
 *
 * This is intentionally a target smoke test rather than the game loop. It
 * proves that reviewed portable modules compile and link under nxdk with
 * their 32-bit layout assertions active.
 */
int main(void)
{
    List list;
    Node node = {0};
    VECTOR vector = {3, 4, 0, 0};
    VECTOR normal = {0, 0, 0, 0};
    MATRIX identity;
    float sine;
    float cosine;
    void *allocation;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    debugPrint("Jedi Power Battles reconstruction\n");

    memory_InitMemorySystem();
    list_InitList(&list);
    list_AddTail(&list, &node);
    allocation = memory_gCalloc(1, 32);
    timer_gSetRoundTimer(1);
    FindSinCos(1024, &sine, &cosine);
    vec_IdentMatrix(&identity);

    if (list_RemoveHead(&list) == &node
        && allocation != NULL
        && timer_gGetRoundTimer() == 3000
        && vec_LengthLV(&vector) == 5
        && VectorNormal(&vector, &normal) == 5
        && normal.vx == 2457
        && normal.vy == 3276
        && normal.vz == 0
        && rsin(1024) == 4095
        && sine > 0.99999f
        && cosine > -0.00001f
        && cosine < 0.00001f
        && identity.m[0][0] == 1.0f
        && identity.m[1][1] == 1.0f
        && identity.m[2][2] == 1.0f
        && identity.t[0] == 0
        && identity.t[1] == 0
        && identity.t[2] == 0) {
        debugPrint("Portable foundation smoke test passed.\n");
    } else {
        debugPrint("Portable foundation smoke test FAILED.\n");
    }

    for (;;) {
        Sleep(1000);
    }
}
