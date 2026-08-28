/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\slots.c.
 * PDB module: 0079
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\slots.obj
 * Primary source: W:\SWJediPowerBattles\Work\slots.c
 * Compiler language: c
 * Emitted procedures: 2
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/slots.h"

#include <limits.h>
#include <string.h>

int maptarget[2];

static int libpartanimtick = 1;
static unsigned char fatTrack[256];
static int bleft;
static int bright;
static int btop;
static int bbottom;
static int wasculled;
static int elapsedleveltime;

/* 0xF7BC0, 438 bytes, global, 9 named locals
 * sampleWalk
 * PDB type: int (cubeStack*, int, int, int, ...
 * Source: W:\SWJediPowerBattles\Work\slots.c
 */
int sampleWalk(
    cubeStack *sr,
    int minx,
    int minz,
    int maxx,
    int maxz,
    MATRIX *matrix)
{
    int x;
    int z;
    int drawn = 0;
    int row_width;
    int row_value;
    int translated_x;

    (void)matrix;
    ++elapsedleveltime;
    if (elapsedleveltime % 6 == 0) {
        libpartanimtick = 3;
    } else {
        libpartanimtick = 2 - (elapsedleveltime % 3 != 0);
    }

    sr->besttargetlen = INT_MAX;
    maptarget[0] = 0;
    maptarget[1] = 0;
    memset(fatTrack, 0, sizeof(fatTrack));

    sr->animTrans.vz = (minz - 127) * 256;
    bleft = (minx + maxx) / 2;
    bright = bleft;
    btop = (minz + maxz) / 2;
    bbottom = btop;

    if (minz <= maxz) {
        translated_x = (129 - minx) * 256;
        row_value = (maxz - minz) * 256 + 256 + sr->animTrans.vz;
        row_width = maxx - minx + 1;

        for (z = minz; z <= maxz; ++z) {
            int current_x = translated_x;

            if (minx <= maxx) {
                wasculled = 0;
                drawn += row_width * row_value;
                for (x = minx; x <= maxx; ++x) {
                    if (x < bleft) {
                        bleft = x;
                    }
                    if (x > bright) {
                        bright = x;
                    }
                    if (z < btop) {
                        btop = z;
                    }
                    if (z > bbottom) {
                        bbottom = z;
                    }
                    current_x -= 256;
                }
            }
            sr->animTrans.vx = current_x;
        }
        sr->animTrans.vz = row_value;
    }
    return drawn;
}

/* 0xF7D80, 21 bytes, global, 0 named locals
 * slot_levelstart
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\Work\slots.c
 */
int slot_levelstart(void)
{
    elapsedleveltime = 0;
    libpartanimtick = 1;
}

#ifdef JPB_SLOTS_TESTING
void jpb_slots_get_state(JPBSlotsState *state)
{
    state->libpartanimtick = libpartanimtick;
    state->bleft = bleft;
    state->bright = bright;
    state->btop = btop;
    state->bbottom = bbottom;
    state->wasculled = wasculled;
    state->elapsedleveltime = elapsedleveltime;
    memcpy(state->fatTrack, fatTrack, sizeof(fatTrack));
}
#endif
