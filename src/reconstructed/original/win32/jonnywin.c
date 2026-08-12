/*
 * PARTIALLY REVIEWED RECONSTRUCTION.
 * PDB module: 0097
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\jonnywin.obj
 * Primary source: W:\SWJediPowerBattles\work\win32\jonnywin.c
 * Compiler language: c
 * Emitted procedures: 4
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/jonnywin.h"

_winmat globalwinmatrix;

/* 0x128980, 1142 bytes, global, 10 named locals
 * RotTransPersMany10bit
 * PDB type: int (int*, int, FVECTOR*)
 * Source: W:\SWJediPowerBattles\work\win32\jonnywin.c
 */

/* 0x128E00, 230 bytes, global, 6 named locals
 * RotTransPersManyFV
 * PDB type: int (FVECTOR*, int, FVECTOR*)
 * Source: W:\SWJediPowerBattles\work\win32\jonnywin.c
 */

/* 0x128EF0, 113 bytes, global, 1 named locals
 * SetupTransformMatrix
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\work\win32\jonnywin.c
 */
void SetupTransformMatrix(MATRIX *matrix)
{
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            globalwinmatrix.m[row][column] = matrix->m[row][column];
        }
    }
    globalwinmatrix.t[0] = (float)matrix->t[0];
    globalwinmatrix.t[1] = (float)matrix->t[1];
    globalwinmatrix.t[2] = (float)matrix->t[2];
}

/* 0x128F70, 190 bytes, global, 1 named locals
 * SetupWorldmeshMatrix
 * PDB type: void (MATRIX*)
 * Source: W:\SWJediPowerBattles\work\win32\jonnywin.c
 */
