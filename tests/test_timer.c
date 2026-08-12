#include "jpb/timer.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                  \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_callbacks(void)
{
    vbls = 41;
    timer_GpuCallback(NULL);
    timer_SuperVbl();
    timer_gInitSystemTimer();
    CHECK(vbls == 41);
    timer_VBlankCallback();
    CHECK(vbls == 42);
    return 0;
}

static int test_round_timer_table_and_conversion(void)
{
    static const int32_t expected[] = {
        -1, 460800, 691200, 921600, 1382400
    };
    static const int32_t expected_display[] = {-1, 3000, 4500, 6000, 9000};
    int index;

    for (index = 0; index < 5; ++index) {
        timer_gSetRoundTimer(index);
        CHECK(mRoundTimer == expected[index]);
        CHECK(timer_gGetRoundTimer() == expected_display[index]);
    }

    mRoundTimer = 12345;
    timer_gSetRoundTimer(-1);
    CHECK(mRoundTimer == 12345);
    timer_gSetRoundTimer(5);
    CHECK(mRoundTimer == 12345);
    return 0;
}

static int test_round_timer_tick(void)
{
    mRoundTimer = -1;
    CHECK(timer_gCheckSystemTimer() == 0);
    CHECK(mRoundTimer == -1);

    mRoundTimer = 513;
    CHECK(timer_gCheckSystemTimer() == 0);
    CHECK(mRoundTimer == 1);
    CHECK(timer_gCheckSystemTimer() == 1);
    CHECK(mRoundTimer == 0);
    CHECK(timer_gCheckSystemTimer() == 0);
    return 0;
}

int main(void)
{
    int result = 0;

    result |= test_callbacks();
    result |= test_round_timer_table_and_conversion();
    result |= test_round_timer_tick();
    return result;
}
