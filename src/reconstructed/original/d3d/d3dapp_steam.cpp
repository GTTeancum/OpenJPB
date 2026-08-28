/*
 * Dependency-isolated exact body from matched-PDB d3dapp module 0020.
 * Kept in its own archive object because only Steam-aware consumers provide
 * SteamInternal_FindOrCreateUserInterface.
 */

#include "jpb/steam_interfaces.h"

/* 0x38BE0, 33 bytes, global, 1 named locals
 * SteamInternal_Init_SteamUtils
 * PDB type: void (ISteamUtils**)
 * Source: W:\SWJediPowerBattles\work\steam\include\isteamutils.h
 */
void SteamInternal_Init_SteamUtils(ISteamUtils **utils)
{
    *utils = static_cast<ISteamUtils *>(
        SteamInternal_FindOrCreateUserInterface(0, "SteamUtils010"));
}
