/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\support.c.
 * PDB module: 0083
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\support.obj
 * Primary source: W:\SWJediPowerBattles\Work\support.c
 * Compiler language: c
 * Emitted procedures: 2
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/support.h"

#include <stdlib.h>

/* 0xFDC00, 36 bytes, global, 1 named locals
 * Random
 * PDB type: int (long)
 * Source: W:\SWJediPowerBattles\Work\support.c
 */
int Random(long range)
{
    if (range == 0) {
        return 0;
    }
    return rand() % (int)range;
}

/* 0xFDC30, 91 bytes, global, 4 named locals
 * f12Random
 * PDB type: int (long)
 * Source: W:\SWJediPowerBattles\Work\support.c
 */
int f12Random(long range)
{
    int first = rand();
    int second = rand();
    int divisor = 32767 / (int)range;

    return
        ((first + 1) / divisor) * 4096 - 4096 +
        (second + 1) / 7;
}
