/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\wRender.c.
 *
 * Provenance:
 *   direct   - exact names, signatures, module-local matrix globals, and
 *              16-entry storage extent from the PDB.
 *   assembly - stack bounds, copy width, and underflow/overflow behavior
 *              checked at RVAs 0x12D500 and 0x12D550.
 *
 * PDB module: 0104
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wRender.obj
 * Primary source: W:\SWJediPowerBattles\Work\wRender.c
 * Compiler language: c
 * Emitted procedures: 11
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/wrender.h"

/* Exact PDB-named wRender.c module locals at RVAs 0x92DAE0..0x92DE18. */
static MATRIX gte_matrix_stack[16];
static int matrix_stack_level;
static MATRIX gte_matrix;

MATRIX *jpb_WRenderCurrentMatrix(void)
{
    return &gte_matrix;
}

int jpb_WRenderMatrixStackLevel(void)
{
    return matrix_stack_level;
}

/* 0x12D500, 70 bytes, global, 0 named locals
 * PopMatrix
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void PopMatrix(void)
{
    if (matrix_stack_level != 0) {
        --matrix_stack_level;
        gte_matrix = gte_matrix_stack[matrix_stack_level];
    }
}

/* 0x12D550, 70 bytes, global, 0 named locals
 * PushMatrix
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
void PushMatrix(void)
{
    if (matrix_stack_level < 15) {
        gte_matrix_stack[matrix_stack_level] = gte_matrix;
        ++matrix_stack_level;
    }
}

/* 0x12D5A0, 916 bytes, global, 17 named locals
 * _CubeRender
 * PDB type: int (wsl_mapEntry*, MATRIX*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12D940, 284 bytes, global, 5 named locals
 * _Cull
 * PDB type: int (MATRIX*, VECTOR*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12DA60, 688 bytes, global, 10 named locals
 * _FatRender
 * PDB type: int (wsl_fatPoly*, MATRIX*, char...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12DD10, 199 bytes, global, 4 named locals
 * _PerspectiveTransform
 * PDB type: void (MATRIX*, _svector*, fPoint...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12DDE0, 1397 bytes, global, 10 named locals
 * _RenderParticle
 * PDB type: void (MATRIX*, PCB*)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12E360, 869 bytes, global, 12 named locals
 * _ThinRender
 * PDB type: int (wsl_thinPoly*, MATRIX*, cha...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12E6D0, 3 bytes, global, 3 named locals
 * __InitDisplay
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12E6E0, 1305 bytes, global, 22 named locals
 * gl_RenderNode
 * PDB type: int (geomData*, MATRIX*, MATRIX*...
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */

/* 0x12EC00, 3 bytes, global, 0 named locals
 * psx_LoadBar
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\wRender.c
 */
