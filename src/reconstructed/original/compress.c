/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\compress.c.
 * PDB module: 0017
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\compress.obj
 * Primary source: W:\SWJediPowerBattles\Work\compress.c
 * Compiler language: c
 * Emitted procedures: 4
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/compress.h"

#include <stdint.h>

static unsigned char *D_Offset;

static int16_t comp_sign_extend(uint16_t value, unsigned bits)
{
    uint16_t mask = (uint16_t)((1u << bits) - 1u);
    uint16_t sign = (uint16_t)(1u << (bits - 1u));

    value &= mask;
    if ((value & sign) != 0) {
        value |= (uint16_t)~mask;
    }
    return (int16_t)value;
}

/* 0x27AF0, 265 bytes, global, 4 named locals
 * comp_AddFrames
 * PDB type: void (_svector*, _svector*, int)
 * Source: W:\SWJediPowerBattles\Work\compress.c
 */
void comp_AddFrames(_svector *last, _svector *pNew, int D_PartCount)
{
    int i;

    for (i = 0; i < D_PartCount + 1; ++i) {
        uint16_t x = (uint16_t)last[i].vx + (uint16_t)pNew[i].vx;
        uint16_t y = (uint16_t)last[i].vy + (uint16_t)pNew[i].vy;
        uint16_t z = (uint16_t)last[i].vz + (uint16_t)pNew[i].vz;

        pNew[i].vx = (int16_t)x;
        pNew[i].vy = (int16_t)y;
        pNew[i].vz = (int16_t)z;
        if ((uint16_t)pNew[i].pad == 0x5aa5u) {
            pNew[i].pad = last[i].pad;
        }
        if (i != 0) {
            pNew[i].vx = comp_sign_extend(x, 12);
            pNew[i].vy = comp_sign_extend(y, 12);
            pNew[i].vz = comp_sign_extend(z, 12);
        }
    }
}

/* 0x27C00, 206 bytes, global, 7 named locals
 * comp_GetDeltaFrame
 * PDB type: unsigned char* (unsigned char*, ...
 * Source: W:\SWJediPowerBattles\Work\compress.c
 */
unsigned char *comp_GetDeltaFrame(
    unsigned char *addr,
    _svector *Dest,
    short first,
    int D_PartCount)
{
    int i;

    D_Offset = addr;
    if (first != 0) {
        Dest[0].vx = (int16_t)((uint16_t)addr[0] | (uint16_t)addr[1] << 8);
        Dest[0].vy = (int16_t)((uint16_t)addr[2] | (uint16_t)addr[3] << 8);
        Dest[0].vz = (int16_t)((uint16_t)addr[4] | (uint16_t)addr[5] << 8);
        Dest[0].pad = (int16_t)((uint16_t)addr[6] | (uint16_t)addr[7] << 8);
        D_Offset = addr + 8;
    }

    for (i = first; i < D_PartCount + 1; ++i) {
        comp_GetSVector(&Dest[i], (short)(i == 0));
    }
    return D_Offset;
}

/* 0x27CD0, 100 bytes, global, 4 named locals
 * comp_GetFrameTrans
 * PDB type: int (unsigned char*, _svector*)
 * Source: W:\SWJediPowerBattles\Work\compress.c
 */
int comp_GetFrameTrans(unsigned char *addr, _svector *Dest)
{
    Dest->vx = (int16_t)((uint16_t)addr[0] | (uint16_t)addr[1] << 8);
    Dest->vy = (int16_t)((uint16_t)addr[2] | (uint16_t)addr[3] << 8);
    Dest->vz = (int16_t)((uint16_t)addr[4] | (uint16_t)addr[5] << 8);
    Dest->pad = (int16_t)((uint16_t)addr[6] | (uint16_t)addr[7] << 8);
    D_Offset = addr + 8;
    return (int)(intptr_t)D_Offset;
}

/* 0x27D40, 1081 bytes, global, 9 named locals
 * comp_GetSVector
 * PDB type: void (_svector*, short)
 * Source: W:\SWJediPowerBattles\Work\compress.c
 */
void comp_GetSVector(_svector *Vect, short pdsz)
{
    unsigned char control = *D_Offset++;
    unsigned char *next = D_Offset;

    switch (control & 0x70u) {
    case 0x00:
        Vect->vx = 0;
        Vect->vy = 0;
        Vect->vz = 0;
        break;

    case 0x10:
        Vect->vx = comp_sign_extend(control, 4);
        Vect->vy = 0;
        Vect->vz = 0;
        break;

    case 0x20:
        Vect->vx = 0;
        Vect->vy = comp_sign_extend(control, 4);
        Vect->vz = 0;
        break;

    case 0x30:
        Vect->vx = 0;
        Vect->vy = 0;
        Vect->vz = comp_sign_extend(control, 4);
        break;

    case 0x40: {
        unsigned char packed = next[0];

        D_Offset = next + 1;
        Vect->vx = comp_sign_extend(control, 4);
        Vect->vy = comp_sign_extend((uint16_t)(packed >> 4), 4);
        Vect->vz = comp_sign_extend(packed, 4);
        break;
    }

    case 0x50: {
        unsigned char byte2 = next[0];
        unsigned char byte3 = next[1];

        D_Offset = next + 2;
        Vect->vx = comp_sign_extend(
            (uint16_t)((uint16_t)control << 3 | byte2 >> 5), 7);
        Vect->vy = comp_sign_extend(
            (uint16_t)((uint16_t)byte2 << 1 | byte3 >> 7), 6);
        Vect->vz = comp_sign_extend(byte3, 7);
        break;
    }

    case 0x60: {
        unsigned char byte2 = next[0];
        unsigned char byte3 = next[1];
        unsigned char byte4 = next[2];

        D_Offset = next + 3;
        Vect->vx = comp_sign_extend(
            (uint16_t)(((control & 0x0fu) << 5) | byte2 >> 3), 9);
        Vect->vy = comp_sign_extend(
            (uint16_t)(((byte2 & 7u) << 6) | byte3 >> 2), 9);
        Vect->vz = comp_sign_extend(
            (uint16_t)(((byte3 & 3u) << 7) | byte4 >> 1), 9);
        break;
    }

    default: {
        unsigned char byte2 = next[0];
        unsigned char byte3 = next[1];
        unsigned char byte4 = next[2];
        unsigned char byte5 = next[3];

        D_Offset = next + 4;
        Vect->vx = comp_sign_extend(
            (uint16_t)((uint16_t)control << 8 | byte2), 12);
        Vect->vy = comp_sign_extend(
            (uint16_t)((uint16_t)byte3 << 4 | byte4 >> 4), 12);
        Vect->vz = comp_sign_extend(
            (uint16_t)((uint16_t)byte4 << 8 | byte5), 12);
        break;
    }
    }

    if ((control & 0x80u) != 0) {
        unsigned char low = *D_Offset++;
        unsigned char high = 0;

        if (pdsz != 0) {
            high = *D_Offset++;
        }
        Vect->pad = (int16_t)((uint16_t)low | (uint16_t)high << 8);
    } else {
        Vect->pad = (int16_t)0x5aa5;
    }
}
