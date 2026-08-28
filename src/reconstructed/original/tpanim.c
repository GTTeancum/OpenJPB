/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\tpanim.c.
 * PDB module: 0088
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\tpanim.obj
 * Primary source: W:\SWJediPowerBattles\Work\tpanim.c
 * Compiler language: c
 * Emitted procedures: 4
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/list.h"
#include "jpb/tpanim.h"

/* Exact PDB globals at matched-PC RVAs 0x581FD0, 0x5823D0, and 0x5823D8. */
int UVCoreIndex[256];
static int mCurUVIndex;
static List mPalList;

/* 0x1031F0, 3 bytes, global, 2 named locals
 * tpanim_AnimateSCBTexture
 * PDB type: void (TexAnim*, SCB*)
 * Source: W:\SWJediPowerBattles\Work\tpanim.c
 */
void tpanim_AnimateSCBTexture(TexAnim *animation, SCB *scb)
{
    (void)animation;
    (void)scb;
}

/* 0x103200, 50 bytes, global, 2 named locals
 * tpanim_RegisterTextureUV
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\tpanim.c
 */
void tpanim_RegisterTextureUV(int texture, int uv)
{
    if (mCurUVIndex < 128) {
        int index = mCurUVIndex * 2;

        ++mCurUVIndex;
        UVCoreIndex[index] = texture;
        UVCoreIndex[index + 1] = uv;
    }
}

/* 0x103240, 11 bytes, global, 0 named locals
 * tpanim_ResetTextureUVIndex
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\tpanim.c
 */
void tpanim_ResetTextureUVIndex(void)
{
    mCurUVIndex = 0;
}

/* 0x103250, 15 bytes, global, 1 named locals
 * tpanim_gAnimatePalette
 * PDB type: void (PalAnim*)
 * Source: W:\SWJediPowerBattles\Work\tpanim.c
 */
void tpanim_gAnimatePalette(PalAnim *animation)
{
    (void)list_AddTail(&mPalList, (Node *)animation);
}
