/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\achievement.cpp.
 *
 * Provenance:
 *   direct     - function names, signatures, source location, and local names
 *                from the exact matching PDB.
 *   decompiled - achievement_complete control flow from the matching game.exe.
 *   assembly   - achievement ID 1 early return, inclusive 2..43 completion
 *                scan, comparison against 1, and final platinum completion
 *                checked at RVA 0x15970.
 *
 * PDB module: 0000
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\achievement.obj
 * Primary source: W:\SWJediPowerBattles\work\achievement.cpp
 * Compiler language: c++
 * Emitted procedures: 6
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/achievement.h"
#include "jpb/platform.h"

/* Exact PDB global at matched-PC RVA 0x4DC340. */
int achievements[JPB_ACHIEVEMENT_COUNT];

/* 0x15930, 64 bytes, global, 2 named locals
 * achievement_checkForPlatinum
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */

/* 0x15970, 60 bytes, global, 2 named locals
 * achievement_complete
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */
void achievement_checkForPlatinum(int id)
{
    int i;

    if (id == 1) {
        return;
    }
    for (i = 2; i < 44; ++i) {
        if (platform_getCompleteAchievement(i) != 1) {
            return;
        }
    }
    (void)platform_completeAchievement(1);
}
void achievement_complete(int id)
{
    int i;

    (void)platform_completeAchievement(id);
    if (id == 1) {
        return;
    }
    for (i = 2; i < 44; ++i) {
        if (platform_getCompleteAchievement(i) != 1) {
            return;
        }
    }
    (void)platform_completeAchievement(1);
}

/* 0x159B0, 3 bytes, global, 0 named locals
 * achievement_destroy
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */
void achievement_destroy(void)
{
}

/* 0x159C0, 5 bytes, global, 1 named locals
 * achievement_getcomplete
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */

/* 0x159D0, 14 bytes, global, 1 named locals
 * achievement_getcount
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */
int achievement_getcomplete(int id)
{
    return platform_getCompleteAchievement(id);
}
int achievement_getcount(int id)
{
    return achievements[id];
}

/* 0x159E0, 14 bytes, global, 2 named locals
 * achievement_update
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\work\achievement.cpp
 */
void achievement_update(int id, int count)
{
    achievements[id] = count;
}
