/*
 * REVIEWED RECONSTRUCTION of the retail compatibility procedures in
 * W:\SWJediPowerBattles\Work\linkstubs.c.
 * PDB module: 0049
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\linkstubs.obj
 * Primary source: W:\SWJediPowerBattles\Work\linkstubs.c
 * Compiler language: c
 * Emitted procedures: 66
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/linkstubs.h"

#include "jpb/jonny.h"
#include "jpb/prim.h"
#include "jpb/sabre.h"
#include "jpb/scene.h"

#include <string.h>

typedef struct DR_TWIN DR_TWIN;

/* 0xBB750, 3 bytes, global, 0 named locals
 * AddPrim
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void AddPrim(void) {}

/* 0xBB760, 3 bytes, global, 0 named locals
 * CdInit
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void CdInit(void) {}

/* 0xBB770, 3 bytes, global, 0 named locals
 * ChangeClearPAD
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void ChangeClearPAD(void) {}

/* 0xBB780, 3 bytes, global, 2 named locals
 * ClearOTagR
 * PDB type: unsigned* (unsigned*, int)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned *ClearOTagR(unsigned *orderingTable, int length)
{
    (void)orderingTable;
    (void)length;
}

/* 0xBB790, 8 bytes, global, 3 named locals
 * CopyMemLong
 * PDB type: void (void*, void*, unsigned lon...
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void CopyMemLong(void *destination, void *source, unsigned long length)
{
    (void)memcpy(destination, source, length);
}

/* 0xBB7A0, 3 bytes, global, 0 named locals
 * DEL_subdivide_gt4_asm
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void DEL_subdivide_gt4_asm(void) {}

/* 0xBB7B0, 3 bytes, global, 0 named locals
 * DrawOTag
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void DrawOTag(void) {}

/* 0xBB7C0, 3 bytes, global, 0 named locals
 * DrawPrim
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void DrawPrim(void *primitive)
{
    (void)primitive;
}

/* 0xBB7D0, 3 bytes, global, 1 named locals
 * DrawSync
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int DrawSync(int mode)
{
    (void)mode;
}

/* 0xBB7E0, 3 bytes, global, 0 named locals
 * ExitCriticalSection
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void ExitCriticalSection(void) {}

/* 0xBB7F0, 3 bytes, global, 0 named locals
 * FntLoad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void FntLoad(void) {}

/* 0xBB800, 3 bytes, global, 2 named locals
 * GetClut
 * PDB type: unsigned short (int, int)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned short GetClut(int x, int y)
{
    (void)x;
    (void)y;
}

/* 0xBB810, 3 bytes, global, 4 named locals
 * GetTPage
 * PDB type: unsigned short (int, int, int, i...
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned short GetTPage(int type, int abr, int x, int y)
{
    (void)type;
    (void)abr;
    (void)x;
    (void)y;
}

/* 0xBB820, 3 bytes, global, 0 named locals
 * InitGeom
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void InitGeom(void) {}

/* 0xBB830, 3 bytes, global, 0 named locals
 * InitTAP
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void InitTAP(void) {}

/* 0xBB840, 3 bytes, global, 0 named locals
 * LoadAddress1
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void LoadAddress1(void) {}

/* 0xBB850, 3 bytes, global, 0 named locals
 * LoadAddress2
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void LoadAddress2(void) {}

/* 0xBB860, 3 bytes, global, 3 named locals
 * LoadClut
 * PDB type: unsigned short (unsigned*, int, ...
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned short LoadClut(unsigned *source, int x, int y)
{
    (void)source;
    (void)x;
    (void)y;
}

/* 0xBB870, 3 bytes, global, 2 named locals
 * LoadImage
 * PDB type: int (SRECT*, unsigned*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int LoadImage(SRECT *rectangle, unsigned *source)
{
    (void)rectangle;
    (void)source;
}

/* 0xBB880, 3 bytes, global, 1 named locals
 * OpenTIM
 * PDB type: int (unsigned*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int OpenTIM(unsigned *address)
{
    (void)address;
    return 0;
}

/* 0xBB890, 3 bytes, global, 1 named locals
 * PutDispEnv
 * PDB type: DISPENV* (DISPENV*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
DISPENV *PutDispEnv(DISPENV *environment)
{
    (void)environment;
}

/* 0xBB8A0, 3 bytes, global, 1 named locals
 * PutDrawEnv
 * PDB type: DRAWENV* (DRAWENV*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
DRAWENV *PutDrawEnv(DRAWENV *environment)
{
    (void)environment;
}

/* 0xBB8B0, 3 bytes, global, 1 named locals
 * ReadTIM
 * PDB type: TIM_IMAGE* (TIM_IMAGE*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
TIM_IMAGE *ReadTIM(TIM_IMAGE *image)
{
    (void)image;
}

/* 0xBB8C0, 3 bytes, global, 0 named locals
 * RenderLibPart
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void RenderLibPart(void) {}

/* 0xBB8D0, 3 bytes, global, 0 named locals
 * SelectMask
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SelectMask(void) {}

/* 0xBB8E0, 3 bytes, global, 0 named locals
 * SetBackColor
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */

/* 0xBB8F0, 3 bytes, global, 0 named locals
 * SetCameraMatrix
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetBackColor(void) {}
void SetCameraMatrix(void)
{
}

/* 0xBB900, 3 bytes, global, 0 named locals
 * SetColorMatrix
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetColorMatrix(void) {}

/* 0xBB910, 3 bytes, global, 5 named locals
 * SetDefDrawEnv
 * PDB type: DRAWENV* (DRAWENV*, int, int, in...
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
DRAWENV *SetDefDrawEnv(
    DRAWENV *environment, int x, int y, int width, int height)
{
    (void)environment;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

/* 0xBB920, 3 bytes, global, 1 named locals
 * SetDispMask
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetDispMask(int enabled)
{
    (void)enabled;
}

/* 0xBB930, 3 bytes, global, 2 named locals
 * SetDrawArea
 * PDB type: void (void*, void*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetDrawArea(void *primitive, void *rectangle)
{
    (void)primitive;
    (void)rectangle;
}

/* 0xBB940, 3 bytes, global, 0 named locals
 * SetDrawMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetDrawMode(void) {}

/* 0xBB950, 3 bytes, global, 0 named locals
 * SetDumpFnt
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetDumpFnt(void) {}

/* 0xBB960, 3 bytes, global, 0 named locals
 * SetPolyFT4
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetPolyFT4(void) {}

/* 0xBB970, 3 bytes, global, 1 named locals
 * SetPolyG4
 * PDB type: void (POLY_G4*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetPolyG4(POLY_G4 *primitive)
{
    (void)primitive;
}

/* 0xBB980, 33 bytes, global, 1 named locals
 * SetRotMatrix
 * PDB type: void (long*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */

void SetRotMatrix(MATRIX *matrix)
{
    gGTEMATRIX = *matrix;
}

/* 0xBB9B0, 3 bytes, global, 0 named locals
 * SetSemiTrans
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetSemiTrans(void *primitive, int enabled)
{
    (void)primitive;
    (void)enabled;
}

/* 0xBB9C0, 3 bytes, global, 0 named locals
 * SetShadeTex
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetShadeTex(void *primitive, int enabled)
{
    (void)primitive;
    (void)enabled;
}

/* 0xBB9D0, 3 bytes, global, 2 named locals
 * SetTexWindow
 * PDB type: void (DR_TWIN*, SRECT*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void SetTexWindow(DR_TWIN *primitive, SRECT *rectangle)
{
    (void)primitive;
    (void)rectangle;
}

/* 0xBB9E0, 33 bytes, global, 1 named locals
 * SetTransMatrix
 * PDB type: void (long*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */

void SetTransMatrix(MATRIX *matrix)
{
    gGTEMATRIX = *matrix;
}

/* 0xBBA10, 3 bytes, global, 2 named locals
 * StoreImage
 * PDB type: int (SRECT*, unsigned*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int StoreImage(SRECT *rectangle, unsigned *destination)
{
    (void)rectangle;
    (void)destination;
}

/* 0xBBA20, 3 bytes, global, 0 named locals
 * VSync
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void VSync(void) {}

/* 0xBBA30, 3 bytes, global, 0 named locals
 * _HandleBackDrop
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void _HandleBackDrop(void)
{
}

/* 0xBBA40, 3 bytes, global, 0 named locals
 * closeFileLog
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void closeFileLog(void) {}

/* 0xBBA50, 3 bytes, global, 0 named locals
 * debug_string
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void debug_string(void) {}

/* 0xBBA60, 3 bytes, global, 0 named locals
 * debug_string3
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void debug_string3(void) {}

/* 0xBBA70, 3 bytes, global, 4 named locals
 * findMTD_TIM
 * PDB type: unsigned (char*, unsigned char*,...
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned findMTD_TIM(
    char *name,
    unsigned char *data,
    unsigned char **result,
    unsigned char *end)
{
    (void)name;
    (void)data;
    (void)result;
    (void)end;
}

/* 0xBBA80, 14 bytes, global, 1 named locals
 * getScratchAddr
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
char *getScratchAddr(int offset)
{
    return (char *)gaScratch + offset;
}

/* 0xBBA90, 3 bytes, global, 0 named locals
 * openFileLog
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void openFileLog(void) {}

/* 0xBBAA0, 3 bytes, global, 1 named locals
 * openXamLog
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void openXamLog(char *filename)
{
    (void)filename;
}

/* 0xBBAB0, 3 bytes, global, 0 named locals
 * platform_boostCPU
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void platform_boostCPU(void) {}

/* 0xBBAC0, 3 bytes, global, 1 named locals
 * platform_completeLevel
 * PDB type: int (char)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int platform_completeLevel(char level)
{
    (void)level;
    return 0;
}

/* 0xBBAD0, 3 bytes, global, 1 named locals
 * platform_enterLevel
 * PDB type: int (char)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int platform_enterLevel(char level)
{
    (void)level;
    return 0;
}

/* 0xBBAE0, 3 bytes, global, 0 named locals
 * platform_isSuspended
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
int platform_isSuspended(void)
{
    return 0;
}

/* 0xBBAF0, 3 bytes, global, 0 named locals
 * platform_unboostCPU
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void platform_unboostCPU(void) {}

/* 0xBBB00, 3 bytes, global, 0 named locals
 * timeGetTime
 * PDB type: unsigned long ()
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
unsigned long timeGetTime(void)
{
}

/* 0xBBB10, 3 bytes, global, 0 named locals
 * vram_CopyClut
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_CopyClut(void) {}

/* 0xBBB20, 3 bytes, global, 0 named locals
 * vram_MakeTranslucentPalette
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_MakeTranslucentPalette(void) {}

/* 0xBBB30, 3 bytes, global, 0 named locals
 * vram_ScalePalette
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_ScalePalette(void) {}

/* 0xBBB40, 3 bytes, global, 0 named locals
 * vram_gAssignClutId
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gAssignClutId(void) {}

/* 0xBBB50, 3 bytes, global, 0 named locals
 * vram_gAssignTPage
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gAssignTPage(void) {}

/* 0xBBB60, 3 bytes, global, 0 named locals
 * vram_gChangeTPageMode
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gChangeTPageMode(void) {}

/* 0xBBB70, 3 bytes, global, 1 named locals
 * vram_gGetOpenPalettePos
 * PDB type: void (void*)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gGetOpenPalettePos(void *position)
{
    (void)position;
}

/* 0xBBB80, 3 bytes, global, 0 named locals
 * vram_gLoadPalette
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gLoadPalette(void) {}

/* 0xBBB90, 3 bytes, global, 0 named locals
 * vram_gLoadTexture
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gLoadTexture(void) {}

/* 0xBBBA0, 3 bytes, global, 0 named locals
 * vram_gResetVram
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\linkstubs.c
 */
void vram_gResetVram(void) {}
