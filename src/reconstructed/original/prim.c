/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * PDB module: 0065
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\prim.obj
 * Primary source: W:\SWJediPowerBattles\Work\prim.c
 * Compiler language: c
 * Emitted procedures: 11
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/prim.h"

#include <stdint.h>

/*
 * The executable writes the same three bytes into its two draw-environment
 * records. Keeping the pair explicit preserves double-buffer state without
 * importing a host graphics structure into this game-owned module.
 */
static CVECTOR jpb_prim_background[2];

const CVECTOR *jpb_PrimGetBackgroundColor(int surface)
{
    if ((unsigned)surface >= 2) {
        return NULL;
    }
    return &jpb_prim_background[surface];
}

/* 0xE8B80, 343 bytes, global, 4 named locals
 * AddBlur
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE8CE0, 102 bytes, global, 1 named locals
 * QuickEnd
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE8D50, 55 bytes, global, 1 named locals
 * QuickStart
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE8D90, 213 bytes, global, 1 named locals
 * initscoredigits
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE8E70, 934 bytes, global, 14 named locals
 * plotscorenumber
 * PDB type: void (VECTOR*, int, unsigned lon...
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE9220, 3 bytes, global, 0 named locals
 * prim_GpuCallback
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE9230, 3 bytes, global, 6 named locals
 * prim_RendSabreEdges
 * PDB type: void (SramFloorStack*, Sabre*, S...
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE9240, 44 bytes, global, 1 named locals
 * prim_SetTextureWindow
 * PDB type: void (RECT*)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE9270, 48 bytes, global, 3 named locals
 * prim_SetTranslucency
 * PDB type: void (unsigned long*, unsigned l...
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE92A0, 189 bytes, global, 2 named locals
 * prim_gRendSabre
 * PDB type: void (Sabre*)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */

/* 0xE9360, 39 bytes, global, 3 named locals
 * prim_gSetBkColor
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\prim.c
 */
void prim_gSetBkColor(int red, int green, int blue)
{
    int surface;

    for (surface = 0; surface < 2; ++surface) {
        jpb_prim_background[surface].r = (uint8_t)red;
        jpb_prim_background[surface].g = (uint8_t)green;
        jpb_prim_background[surface].b = (uint8_t)blue;
    }
}
