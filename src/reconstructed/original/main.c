#include "jpb/console.h"
#include "jpb/main.h"

/*
 * COMPLETE REVIEWED RECONSTRUCTION
 * PDB module: 0052
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\main.obj
 * Primary source: W:\SWJediPowerBattles\Work\main.c
 * Compiler language: c
 * Emitted procedures: 4
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

int debug_slomo;
int debug_singlestep;

/* 0xBE0D0, 3 bytes, global, 0 named locals
 * card_gInitVariables
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\Work\main.c
 */
int card_gInitVariables(void)
{
    /* The shipped three-byte body returns without defining EAX. */
}

/* 0xBE0E0, 70 bytes, global, 4 named locals
 * console_CommandPause
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\main.c
 */
int console_CommandPause(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;

    if (narg == 0)
    {
        debug_slomo = !debug_slomo;
        return debug_slomo;
    }

    console_Printf("PAUSE - [un]freeze the game (see step)\n");
    console_Printf("usage: pause\n");
    return console_Printf(
        "pause and step are only useful when bound to keys...\n");
}

/* 0xBE130, 104 bytes, global, 4 named locals
 * console_CommandStep
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\Work\main.c
 */
int console_CommandStep(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_float;

    if (narg > 1 || (narg == 1 && arg_str[0][0] == '?'))
    {
        console_Printf("STEP - singlestep the game when frozen (see pause)\n");
        console_Printf("has no effect if game not frozen\n");
        console_Printf("usage: step [frames]\n");
        return console_Printf(
            "pause and step are only useful when bound to keys...\n");
    }

    if (narg != 0)
    {
        debug_singlestep = arg_int[0];
        return debug_singlestep;
    }

    debug_singlestep = 1;
    /* Retail leaves the return register undefined on this path. */
}

/* 0xBE1A0, 155 bytes, global, 0 named locals
 * initialize_main
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\main.c
 * Exact body is dependency-isolated in main_initialize.c so the terminal
 * SteamAPI_Shutdown import is pulled only by consumers of this entry point.
 */
