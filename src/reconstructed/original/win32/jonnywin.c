/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\work\win32\jonnywin.c.
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
int RotTransPersMany10bit(
    int *source, int count, FVECTOR *destination)
{
    int index;

    for (index = 0; index < count; ++index) {
        int packed = source[index];
        int x = (int32_t)((uint32_t)packed << 22) >> 22;
        int y = (int32_t)((uint32_t)packed << 12) >> 22;
        int z = (int32_t)((uint32_t)packed << 2) >> 22;

        destination[index].vx =
            (float)x * globalwinmatrix.m[0][0] +
            (float)y * globalwinmatrix.m[0][1] +
            (float)z * globalwinmatrix.m[0][2] +
            globalwinmatrix.t[0];
        destination[index].vy =
            (float)x * globalwinmatrix.m[1][0] +
            (float)y * globalwinmatrix.m[1][1] +
            (float)z * globalwinmatrix.m[1][2] +
            globalwinmatrix.t[1];
        destination[index].vz =
            (float)x * globalwinmatrix.m[2][0] +
            (float)y * globalwinmatrix.m[2][1] +
            (float)z * globalwinmatrix.m[2][2] +
            globalwinmatrix.t[2];
    }
    return 0;
}

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
int RotTransPersManyFV(
    FVECTOR *source, int count, FVECTOR *destination)
{
    while (count != 0) {
        float x = source->vx;
        float y = source->vy;
        float z = source->vz;

        destination->vx =
            x * globalwinmatrix.m[0][0] +
            y * globalwinmatrix.m[0][1] +
            z * globalwinmatrix.m[0][2] +
            globalwinmatrix.t[0];
        destination->vy =
            x * globalwinmatrix.m[1][0] +
            y * globalwinmatrix.m[1][1] +
            z * globalwinmatrix.m[1][2] +
            globalwinmatrix.t[1];
        destination->vz =
            x * globalwinmatrix.m[2][0] +
            y * globalwinmatrix.m[2][1] +
            z * globalwinmatrix.m[2][2] +
            globalwinmatrix.t[2];

        ++source;
        ++destination;
        --count;
    }
    return 0;
}
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
void SetupWorldmeshMatrix(MATRIX *matrix)
{
    const float worldmeshScale = 256.0f;

    matrix->m[0][0] *= worldmeshScale;
    matrix->m[0][1] *= worldmeshScale;
    matrix->m[0][2] *= worldmeshScale;
    matrix->m[1][0] *= worldmeshScale;
    matrix->m[1][1] *= worldmeshScale;
    matrix->m[1][2] *= worldmeshScale;
    matrix->m[2][0] *= worldmeshScale;
    matrix->m[2][1] *= worldmeshScale;
    matrix->m[2][2] *= worldmeshScale;

    matrix->t[0] = (int)((float)matrix->t[0] * worldmeshScale);
    matrix->t[1] = (int)((float)matrix->t[1] * worldmeshScale);
    matrix->t[2] = (int)((float)matrix->t[2] * worldmeshScale);
}
