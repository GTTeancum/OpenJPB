#ifndef JPB_TIMER_H
#define JPB_TIMER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Direct PDB type 0x9A1F. Fixed-width fields are intentional: Microsoft long
 * is 32-bit in both the x64 reference and the 32-bit Xbox target.
 */
typedef struct _graph_entry {
    uint32_t amount;
    int32_t maxamount;
    int32_t hangtime;
    uint32_t color;
} _graph_entry;

extern int32_t mRoundTimer;
extern _graph_entry timerbucket;
extern int32_t buckettime;
extern int32_t gputiming;
extern volatile uint32_t vbls;

void timer_GpuCallback(void *args);
void timer_SuperVbl(void);
void timer_VBlankCallback(void);
int timer_gCheckSystemTimer(void);
int32_t timer_gGetRoundTimer(void);
void timer_gInitSystemTimer(void);
void timer_gSetRoundTimer(int val);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_TIMER_STATIC_ASSERT(condition, message) static_assert(condition, message)
#else
#define JPB_TIMER_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#endif

JPB_TIMER_STATIC_ASSERT(sizeof(_graph_entry) == 16, "_graph_entry size changed");
JPB_TIMER_STATIC_ASSERT(
    offsetof(_graph_entry, amount) == 0, "_graph_entry.amount layout changed");
JPB_TIMER_STATIC_ASSERT(
    offsetof(_graph_entry, maxamount) == 4,
    "_graph_entry.maxamount layout changed");
JPB_TIMER_STATIC_ASSERT(
    offsetof(_graph_entry, hangtime) == 8,
    "_graph_entry.hangtime layout changed");
JPB_TIMER_STATIC_ASSERT(
    offsetof(_graph_entry, color) == 12, "_graph_entry.color layout changed");

#undef JPB_TIMER_STATIC_ASSERT

#endif
