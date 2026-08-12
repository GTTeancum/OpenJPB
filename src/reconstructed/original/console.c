/*
 * PARTIAL REVIEWED RECONSTRUCTION of
 * PDB module: 0018
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\console.obj
 * Primary source: W:\SWJediPowerBattles\work\console.c
 * Compiler language: c
 * Emitted procedures: 38
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/console.h"

#include <stdint.h>
#include <string.h>

typedef struct ConsoleCommand {
    char *name;
    char *shortname;
    ConsoleCommandHandler handler;
} ConsoleCommand;

enum { JPB_CONSOLE_COMMAND_CAPACITY = 255 };

static ConsoleCommand console_command[JPB_CONSOLE_COMMAND_CAPACITY];
static int console_numcommands;

static int console_ascii_casecmp(const char *left, const char *right)
{
    for (;;) {
        unsigned char a = (unsigned char)*left++;
        unsigned char b = (unsigned char)*right++;

        if (a >= 'A' && a <= 'Z') {
            a = (unsigned char)(a + ('a' - 'A'));
        }
        if (b >= 'A' && b <= 'Z') {
            b = (unsigned char)(b + ('a' - 'A'));
        }
        if (a != b || a == '\0') {
            return (int)a - (int)b;
        }
    }
}

void jpb_ConsoleResetCommands(void)
{
    memset(console_command, 0, sizeof(console_command));
    console_numcommands = 0;
}

size_t jpb_ConsoleCommandCount(void)
{
    return (size_t)console_numcommands;
}

const char *jpb_ConsoleCommandName(size_t index)
{
    return index < (size_t)console_numcommands
        ? console_command[index].name
        : NULL;
}

const char *jpb_ConsoleCommandShortName(size_t index)
{
    return index < (size_t)console_numcommands
        ? console_command[index].shortname
        : NULL;
}

ConsoleCommandHandler jpb_ConsoleCommandHandler(size_t index)
{
    return index < (size_t)console_numcommands
        ? console_command[index].handler
        : NULL;
}

/* 0x28180, 100 bytes, global, 3 named locals
 * ascii2hex
 * PDB type: unsigned long (char*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x281F0, 171 bytes, global, 4 named locals
 * console_AddCommand
 * PDB type: int (char*, char*, int (int, cha...
 * Source: W:\SWJediPowerBattles\work\console.c
 */
int console_AddCommand(
    char *name,
    char *shortname,
    ConsoleCommandHandler handler)
{
    int index;

    for (index = 0; index < console_numcommands; ++index) {
        if (console_ascii_casecmp(name, console_command[index].name) == 0) {
            return 1;
        }
    }
    if (console_numcommands < JPB_CONSOLE_COMMAND_CAPACITY) {
        ConsoleCommand *command = &console_command[console_numcommands++];

        command->name = name;
        command->shortname = shortname;
        command->handler = handler;
    }
    return 0;
}

/* 0x282A0, 163 bytes, local, 3 named locals
 * console_AddString
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28350, 131 bytes, global, 1 named locals
 * console_BackSpace
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x283E0, 230 bytes, global, 2 named locals
 * console_ClearCommandLine
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x284D0, 76 bytes, global, 0 named locals
 * console_Cls
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28520, 145 bytes, global, 5 named locals
 * console_CommandCard
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x285C0, 81 bytes, global, 4 named locals
 * console_CommandCls
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28620, 308 bytes, global, 7 named locals
 * console_CommandColor
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28760, 148 bytes, global, 4 named locals
 * console_CommandDelete
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28800, 129 bytes, global, 5 named locals
 * console_CommandDir
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28890, 64 bytes, global, 4 named locals
 * console_CommandDump
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x288D0, 60 bytes, global, 4 named locals
 * console_CommandExit
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28910, 159 bytes, global, 5 named locals
 * console_CommandHelp
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x289B0, 330 bytes, global, 6 named locals
 * console_CommandHistory
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28B00, 657 bytes, global, 9 named locals
 * console_CommandKey
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28DA0, 97 bytes, global, 6 named locals
 * console_CommandList
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28E10, 428 bytes, global, 9 named locals
 * console_CommandLoadKeys
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x28FC0, 60 bytes, global, 4 named locals
 * console_CommandLoadProgram
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29000, 54 bytes, global, 4 named locals
 * console_CommandNew
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29040, 61 bytes, global, 4 named locals
 * console_CommandRun
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29080, 1186 bytes, global, 8 named locals
 * console_CommandSaveKeys
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29530, 60 bytes, global, 4 named locals
 * console_CommandSaveProgram
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29570, 358 bytes, global, 1 named locals
 * console_Display
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x296E0, 43 bytes, global, 4 named locals
 * console_EnableConsoleCommand
 * PDB type: int (int, char**, int*, float*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29710, 211 bytes, global, 2 named locals
 * console_Execute
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x297F0, 1581 bytes, global, 6 named locals
 * console_HandleConsole
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29E20, 11 bytes, global, 0 named locals
 * console_HideConsole
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x29E30, 2092 bytes, local, 19 named locals
 * console_Init
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2A660, 28 bytes, global, 0 named locals
 * console_NabKeyboard
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2A680, 81 bytes, global, 1 named locals
 * console_NextLine
 * PDB type: int (char)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2A6E0, 147 bytes, global, 3 named locals
 * console_Printf
 * PDB type: int (char*, <no type>)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2A780, 617 bytes, global, 7 named locals
 * console_ProcessCommandLine
 * PDB type: int (char*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2A9F0, 13 bytes, global, 0 named locals
 * console_ReleaseKeyboard
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2AA00, 173 bytes, global, 2 named locals
 * console_ResetCommandLine
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2AAB0, 11 bytes, global, 0 named locals
 * console_ShowConsole
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2AAC0, 54 bytes, local, 2 named locals
 * findbinding
 * PDB type: _console_keybinding* (unsigned c...
 * Source: W:\SWJediPowerBattles\work\console.c
 */

/* 0x2AB00, 146 bytes, local, 2 named locals
 * findkey
 * PDB type: unsigned char (char*)
 * Source: W:\SWJediPowerBattles\work\console.c
 */
