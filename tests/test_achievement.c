#include "jpb/achievement.h"
#include "jpb/platform.h"

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

typedef struct AchievementTrace {
    int complete[JPB_ACHIEVEMENT_COUNT];
    int completeCalls;
    int completedIds[4];
    int queryCalls;
    int lastQuery;
} AchievementTrace;

static int trace_complete(int id, void *user_data)
{
    AchievementTrace *trace = (AchievementTrace *)user_data;

    trace->completedIds[trace->completeCalls++] = id;
    trace->complete[id] = 1;
    return 1;
}

static int trace_get_complete(int id, void *user_data)
{
    AchievementTrace *trace = (AchievementTrace *)user_data;

    ++trace->queryCalls;
    trace->lastQuery = id;
    return trace->complete[id];
}

int main(void)
{
    const JPBPlatformAchievementHooks hooks = {
        trace_complete,
        trace_get_complete
    };
    AchievementTrace trace;
    int i;

    memset(&trace, 0, sizeof(trace));
    jpb_PlatformSetAchievementHooks(&hooks, &trace);

    achievement_checkForPlatinum(1);
    CHECK(trace.queryCalls == 0);
    CHECK(trace.completeCalls == 0);

    for (i = 2; i < JPB_ACHIEVEMENT_COUNT; ++i) {
        trace.complete[i] = 1;
    }
    trace.complete[5] = 0;
    achievement_checkForPlatinum(7);
    CHECK(trace.queryCalls == 4);
    CHECK(trace.lastQuery == 5);
    CHECK(trace.completeCalls == 0);

    trace.complete[5] = 1;
    trace.queryCalls = 0;
    achievement_checkForPlatinum(7);
    CHECK(trace.queryCalls == 42);
    CHECK(trace.completeCalls == 1);
    CHECK(trace.completedIds[0] == 1);

    trace.completeCalls = 0;
    trace.queryCalls = 0;
    achievement_complete(9);
    CHECK(trace.completeCalls == 2);
    CHECK(trace.completedIds[0] == 9);
    CHECK(trace.completedIds[1] == 1);
    CHECK(trace.queryCalls == 42);

    trace.completeCalls = 0;
    trace.queryCalls = 0;
    achievement_complete(1);
    CHECK(trace.completeCalls == 1);
    CHECK(trace.completedIds[0] == 1);
    CHECK(trace.queryCalls == 0);

    trace.complete[12] = 1;
    CHECK(achievement_getcomplete(12) == 1);
    CHECK(trace.lastQuery == 12);

    achievement_update(3, 77);
    CHECK(achievement_getcount(3) == 77);
    achievement_destroy();
    jpb_PlatformSetAchievementHooks(NULL, NULL);

    puts("achievement tests passed");
    return 0;
}
