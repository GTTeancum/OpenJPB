/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\timer.c.
 *
 * Provenance:
 *   direct     - function/global names, signatures, types, addresses, and the
 *                five-entry timer table from the exact PDB and executable.
 *   assembly   - all seven bodies checked instruction-by-instruction at RVAs
 *                0x103110 through 0x1031E3, including 32-bit multiply wrap.
 *
 * This module contains the original platform-neutral timer state transitions.
 * Wiring timer_VBlankCallback to the Xbox vertical-blank source belongs in
 * the nxdk platform layer.
 */

#include "jpb/timer.h"

/* Direct global symbols at reference RVAs 0x94475C through 0x944778. */
int32_t mRoundTimer;
_graph_entry timerbucket;
int32_t buckettime;
int32_t gputiming;
volatile uint32_t vbls;

/* Reference RVA 0x103110, 3 bytes. */
void timer_GpuCallback(void *args)
{
    (void)args;
}

/* Reference RVA 0x103120, 3 bytes. */
void timer_SuperVbl(void)
{
}

/* Reference RVA 0x103130, 15 bytes. */
void timer_VBlankCallback(void)
{
    ++vbls;
}

/* Reference RVA 0x103140, 44 bytes. */
int timer_gCheckSystemTimer(void)
{
    if (mRoundTimer > 0) {
        mRoundTimer -= 0x200;
        if (mRoundTimer <= 0) {
            mRoundTimer = 0;
            return 1;
        }
    }
    return 0;
}

/* Reference RVA 0x103170, 39 bytes. */
int32_t timer_gGetRoundTimer(void)
{
    int32_t scaled;

    if (mRoundTimer < 0) {
        return -1;
    }
    scaled = (int32_t)((uint32_t)mRoundTimer * UINT32_C(100));
    return scaled / 0x3C00;
}

/* Reference RVA 0x1031A0, 3 bytes. */
void timer_gInitSystemTimer(void)
{
}

/* Reference RVA 0x1031B0, 52 bytes. */
void timer_gSetRoundTimer(int val)
{
    const int32_t TimeTable[5] = {
        -1,
        460800,
        691200,
        921600,
        1382400,
    };

    if (val >= 0 && val < 5) {
        mRoundTimer = TimeTable[val];
    }
}
