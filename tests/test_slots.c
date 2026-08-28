#include "jpb/slots.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int check_normal_rectangle(void)
{
    cubeStack stack;
    JPBSlotsState state;
    int drawn;
    int index;

    memset(&stack, 0x5a, sizeof(stack));
    maptarget[0] = 17;
    maptarget[1] = 23;
    drawn = sampleWalk(&stack, 2, 4, 5, 6, NULL);

    CHECK(drawn == 3 * 4 * ((6 - 126) * 256));
    CHECK(stack.besttargetlen == INT_MAX);
    CHECK(stack.animTrans.vx == (129 - 2) * 256 - 4 * 256);
    CHECK(stack.animTrans.vz == (6 - 126) * 256);
    CHECK(maptarget[0] == 0);
    CHECK(maptarget[1] == 0);

    jpb_slots_get_state(&state);
    CHECK(state.libpartanimtick == 1);
    CHECK(state.elapsedleveltime == 1);
    CHECK(state.bleft == 2);
    CHECK(state.bright == 5);
    CHECK(state.btop == 4);
    CHECK(state.bbottom == 6);
    CHECK(state.wasculled == 0);
    for (index = 0; index < 256; ++index) {
        CHECK(state.fatTrack[index] == 0);
    }
    return 0;
}

static int check_tick_cycle_and_empty_ranges(void)
{
    cubeStack stack;
    JPBSlotsState state;
    int expected_ticks[6] = {1, 1, 2, 1, 1, 3};
    int frame;

    memset(&stack, 0, sizeof(stack));
    (void)slot_levelstart();
    for (frame = 0; frame < 6; ++frame) {
        CHECK(sampleWalk(&stack, 7, 9, 6, 8, NULL) == 0);
        jpb_slots_get_state(&state);
        CHECK(state.libpartanimtick == expected_ticks[frame]);
        CHECK(state.elapsedleveltime == frame + 1);
    }

    stack.animTrans.vx = 12345;
    CHECK(sampleWalk(&stack, -4, 5, 3, 4, NULL) == 0);
    CHECK(stack.animTrans.vx == 12345);
    CHECK(stack.animTrans.vz == (5 - 127) * 256);

    stack.animTrans.vx = 0;
    CHECK(sampleWalk(&stack, 5, 2, 3, 4, NULL) == 0);
    CHECK(stack.animTrans.vx == (129 - 5) * 256);
    CHECK(stack.animTrans.vz == (4 - 126) * 256);
    return 0;
}

int main(void)
{
    (void)slot_levelstart();
    CHECK(check_normal_rectangle() == 0);
    CHECK(check_tick_cycle_and_empty_ranges() == 0);
    puts("slot walk-sampling tests passed");
    return 0;
}
