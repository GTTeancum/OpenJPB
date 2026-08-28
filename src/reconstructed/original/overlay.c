/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\Work\overlay.c.
 * The matched executable emits a bare return for all ten procedures. There
 * are no direct executable callers, and ovrlay_CheckForce intentionally does
 * not synthesize a value for its PDB-declared int return type.
 * PDB module: 0059
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\overlay.obj
 * Primary source: W:\SWJediPowerBattles\Work\overlay.c
 * Compiler language: c
 * Emitted procedures: 10
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

/* 0xDA6B0, 3 bytes, global, 2 named locals
 * ovrlay_CheckForce
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
int ovrlay_CheckForce(int player, int amount)
{
    (void)player;
    (void)amount;
}

/* 0xDA6C0, 3 bytes, global, 0 named locals
 * ovrlay_DoAlphaScreen
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_DoAlphaScreen(void)
{
}

/* 0xDA6D0, 3 bytes, global, 0 named locals
 * ovrlay_DoWork
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_DoWork(void)
{
}

/* 0xDA6E0, 3 bytes, global, 2 named locals
 * ovrlay_InitOverLay
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_InitOverLay(int player, int mode)
{
    (void)player;
    (void)mode;
}

/* 0xDA6F0, 3 bytes, global, 3 named locals
 * ovrlay_SetForceBar
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_SetForceBar(int player, int value, int maximum)
{
    (void)player;
    (void)value;
    (void)maximum;
}

/* 0xDA700, 3 bytes, global, 3 named locals
 * ovrlay_SetForceGem
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_SetForceGem(int player, int value, int maximum)
{
    (void)player;
    (void)value;
    (void)maximum;
}

/* 0xDA710, 3 bytes, global, 3 named locals
 * ovrlay_SetLifeBar
 * PDB type: void (int, int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_SetLifeBar(int player, int value, int maximum)
{
    (void)player;
    (void)value;
    (void)maximum;
}

/* 0xDA720, 3 bytes, global, 2 named locals
 * ovrlay_SetStunBar
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_SetStunBar(int player, int value)
{
    (void)player;
    (void)value;
}

/* 0xDA730, 3 bytes, global, 2 named locals
 * ovrlay_gSetBarColor
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_gSetBarColor(int player, int color)
{
    (void)player;
    (void)color;
}

/* 0xDA740, 3 bytes, global, 1 named locals
 * ovrlay_gToggleOverlay
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\overlay.c
 */
void ovrlay_gToggleOverlay(int enabled)
{
    (void)enabled;
}
