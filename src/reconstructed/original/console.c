/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * PDB module: 0018
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\console.obj
 * Primary source: W:\SWJediPowerBattles\work\console.c
 * Compiler language: c
 * Emitted procedures: 38
 *
 * Layouts, globals, strings, and control flow are recovered from the matching
 * game.pdb and shipped game.exe. The old C implicit-return behavior at the
 * memcard boundary is deliberately retained where the retail caller consumes
 * the Win32 result left in EAX.
 */

#include "jpb/console.h"

#include "jpb/debugtext.h"
#include "jpb/main.h"
#include "jpb/pwrup.h"
#include "jpb/whook.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CONSOLE_COMMAND_SLOTS = 256,
    CONSOLE_USABLE_COMMANDS = 255,
    CONSOLE_LINES = 256,
    CONSOLE_LINE_BYTES = 235,
    CONSOLE_HISTORY_LINES = 16,
    CONSOLE_COMMAND_BYTES = 256,
    CONSOLE_BINDINGS = 64
};

typedef struct ConsoleInfoHeader {
    int console_alpha;
    int console_ylines;
    int numkeybindings;
} ConsoleInfoHeader;

void basic_AddLine(char *line);
void basic_Init(void);
void basic_List(int from, int to);
void basic_LoadProgram(char *filename, int card);
void basic_New(void);
int basic_PrepareToRun(void);
int basic_Run(int cycles);
void basic_SaveProgram(char *filename, int card);

void memcard_on(void);
void memcard_off(void);
int memcard_FileExists(int card, char *filename);
int memcard_DeleteFile(int card, char *filename);
char *memcard_FindFirstFile(int card);
char *memcard_FindNextFile(int card);
int memcard_LoadFile(
    int card, char *filename, unsigned char **data, unsigned long *size);
void memcard_SaveFile(
    int card, char *filename, unsigned char *data, unsigned long size);

int console_currentmemcard;
int bConsoleActive;
int console_basicrunning;
int bKeyboardNabbed;
int console_enabled;

static char console_key_space[] = " SPACE";
static char console_key_return[] = "\rRETURN";
static char console_key_tab[] = "\tTAB";
static char console_key_escape[] = "\x1b" "ESCAPE";
static char console_key_f1[] = "\xf1" "F1";
static char console_key_f2[] = "\xf2" "F2";
static char console_key_f3[] = "\xf3" "F3";
static char console_key_f4[] = "\xf4" "F4";
static char console_key_f5[] = "\xf5" "F5";
static char console_key_f6[] = "\xf6" "F6";
static char console_key_f7[] = "\xf7" "F7";
static char console_key_f8[] = "\xf8" "F8";
static char console_key_f9[] = "\xf9" "F9";
static char console_key_f10[] = "\xfa" "F10";
static char console_key_f11[] = "\xfb" "F11";
static char console_key_f12[] = "\xfc" "F12";

static unsigned long console_color[3] = {
    0x00f0f0f0UL,
    0x00000000UL,
    0x00f00020UL
};
static char *console_keynames[17] = {
    console_key_space,
    console_key_return,
    console_key_tab,
    console_key_escape,
    console_key_f1,
    console_key_f2,
    console_key_f3,
    console_key_f4,
    console_key_f5,
    console_key_f6,
    console_key_f7,
    console_key_f8,
    console_key_f9,
    console_key_f10,
    console_key_f11,
    console_key_f12,
    NULL
};

static char console_name[2];
static char consolebuffer[CONSOLE_LINES][CONSOLE_LINE_BYTES];
static unsigned console_x;
static unsigned console_y;
static unsigned console_commandline;
static int console_yscroll;
static int console_yscrolldest;
static int console_ylines;
static char console_history[CONSOLE_HISTORY_LINES][CONSOLE_COMMAND_BYTES];
static unsigned console_historypos;
static unsigned console_historybrowser;
static int console_alpha;
static ConsoleCommand console_command[CONSOLE_COMMAND_SLOTS];
static int console_numcommands;
static char *commandargs[16];
static float commandfloat[16];
static int commandint[16];
static char commandbuffer[CONSOLE_COMMAND_BYTES];
static int cursortimer;
static ConsoleKeyBinding console_keybinding[CONSOLE_BINDINGS];
static int bInited;

void console_AddString(char *string);
static ConsoleKeyBinding *findbinding(unsigned char key);
static unsigned char findkey(char *string);
static void console_Init(void);

/* 0x28180, 100 bytes */
unsigned long ascii2hex(char *string)
{
    char c = *string;
    unsigned long value = 0;

    for (;;) {
        unsigned long digit;

        if (c == '\0') {
            return value;
        }
        c = (char)toupper((unsigned char)c);
        value <<= 4;
        if ((unsigned char)(c - '0') < 10) {
            digit = (unsigned long)(c - '0');
        } else if ((unsigned char)(c - 'A') < 6) {
            digit = (unsigned long)(c - 'A' + 10);
        } else {
            return value;
        }
        value |= digit;
        c = *++string;
    }
}

/* 0x281F0, 171 bytes */
int console_AddCommand(
    char *name,
    char *shortname,
    ConsoleCommandHandler handler)
{
    int i;

    for (i = 0; i < console_numcommands; ++i) {
        if (_stricmp(name, console_command[i].name) == 0) {
            return 1;
        }
    }
    if (console_numcommands < CONSOLE_USABLE_COMMANDS) {
        console_command[console_numcommands].name = name;
        console_command[console_numcommands].shortname = shortname;
        console_command[console_numcommands].handler = handler;
        ++console_numcommands;
    }
    return 0;
}

/* 0x282A0, 163 bytes */
void console_AddString(char *string)
{
    char c = *string;

    while (c != '\0') {
        ++string;
        if (c == '\n' || c == '\r') {
            console_y = (unsigned char)(console_y + 1);
            console_x = 0;
        } else {
            if (console_x >= 234) {
                console_x = 0;
                console_y = (unsigned char)(console_y + 1);
                consolebuffer[console_y][0] = '\0';
            }
            consolebuffer[console_y][console_x++] = c;
        }
        consolebuffer[console_y][console_x] = '\0';
        c = *string;
    }
}

/* 0x28350, 131 bytes */
void console_BackSpace(void)
{
    int line_minimum = console_y == console_commandline;

    if (console_x == (unsigned)line_minimum) {
        if (console_commandline < console_y) {
            consolebuffer[console_y][0] = '\0';
            console_y = (unsigned char)(console_y - 1);
            console_x = (unsigned)strlen(consolebuffer[console_y]);
        }
    } else {
        --console_x;
        consolebuffer[console_y][console_x] = '\0';
    }
}

/* 0x283E0, 230 bytes */
void console_ClearCommandLine(void)
{
    while (console_y != console_commandline) {
        consolebuffer[console_y][0] = '\0';
        console_y = (unsigned char)(console_y - 1);
    }
    consolebuffer[console_y][0] = '\0';
    console_x = 0;
    console_AddString("]");
    console_commandline = console_y;
}

/* 0x284D0, 76 bytes */
void console_Cls(void)
{
    int i;

    for (i = 0; i < CONSOLE_LINES; ++i) {
        consolebuffer[i][0] = '\0';
    }
    console_x = 0;
    console_y = (unsigned char)(console_y + 1);
    console_commandline = console_y;
    consolebuffer[console_y][0] = '\0';
}

/* 0x28520, 145 bytes */
int console_CommandCard(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_float;
    if (narg < 2) {
        if (narg == 0) {
            console_Printf(
                "Memcard is currently %d\n", console_currentmemcard);
            return 1;
        }
        if (arg_str[0][0] != '?') {
            if ((unsigned char)(arg_str[0][0] - '0') < 8) {
                console_currentmemcard = arg_int[0];
                console_Printf(
                    "Set memcard to %d\n", console_currentmemcard);
                return 1;
            }
            console_Printf("Range is 0-7, dummy!\n");
            return 1;
        }
    }
    console_Printf("CARD - set/show current memcard\n");
    console_Printf("usage: card [card]\n");
    return 1;
}

/* 0x285C0, 81 bytes */
int console_CommandCls(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    console_Cls();
    return 1;
}

/* 0x28620, 308 bytes */
int console_CommandColor(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    int i;

    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] == '?') {
        console_Printf("COLOR - set console colors\n");
        console_Printf(
            "usage: color [[[textcolor] backgroundcolor] cursorcolor] "
            "(RGB)\n");
        console_Printf("eg: color ffff00 ff00ff ffff00\n");
        return 1;
    }
    if (narg == 0) {
        console_Printf(
            "Colors are: %08x %08x %08x\n",
            console_color[0],
            console_color[1],
            console_color[2]);
        return 1;
    }
    for (i = 0; i < narg; ++i) {
        console_color[i] = ascii2hex(arg_str[i]) & 0x00ffffffUL;
        if (bConsoleActive != 0) {
            console_Printf("%08x\n", console_color[i]);
        }
    }
    return 1;
}

/* 0x28760, 148 bytes */
int console_CommandDelete(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] != '?') {
        memcard_on();
        if (memcard_FileExists(console_currentmemcard, arg_str[0]) == 0) {
            console_Printf("No such file: %s\n", arg_str[0]);
        } else if (memcard_DeleteFile(
                       console_currentmemcard, arg_str[0]) != 0) {
            console_Printf("%s deleted\n", arg_str[0]);
        } else {
            console_Printf("Error deleting %s\n", arg_str[0]);
        }
        memcard_off();
    } else {
        console_Printf("DELETE - delete file from memcard\n");
        console_Printf("usage: delete [filename]\n");
    }
}

/* 0x28800, 129 bytes */
int console_CommandDir(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    char *filename;

    (void)arg_float;
    if (narg > 1 || (narg == 1 && arg_str[0][0] == '?')) {
        console_Printf("DIR - show files on a memcard\n");
        console_Printf("usage: dir [card]\n");
        return 1;
    }
    if (narg == 1) {
        console_currentmemcard = arg_int[0];
    }
    memcard_on();
    filename = memcard_FindFirstFile(console_currentmemcard);
    while (filename != NULL) {
        console_Printf("%s\n", filename);
        filename = memcard_FindNextFile(console_currentmemcard);
    }
    memcard_off();
    return 1;
}

/* 0x28890, 64 bytes */
int console_CommandDump(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    if (narg != 0) {
        console_Printf(
            "DUMP: dump contents of console to debugger output!\n");
        console_Printf("usage: dump\n");
    } else {
        console_Printf("Only in DEBUGVERSION!\n");
    }
    return 1;
}

/* 0x288D0, 60 bytes */
int console_CommandExit(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] == '?') {
        console_Printf("EXIT - close the console\n");
        console_Printf("usage: exit\n");
    } else {
        bConsoleActive = 0;
    }
}

/* 0x28910, 159 bytes */
int console_CommandHelp(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    int i;

    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    console_Printf("Commands:\n");
    for (i = 0; i < console_numcommands; ++i) {
        if (strlen(console_command[i].name) + console_x > 227) {
            console_Printf("\n");
        }
        console_Printf("  %s", console_command[i].name);
    }
    console_Printf("\n\ntype command ? for help on that command\n");
    return 1;
}

/* 0x289B0, 330 bytes */
int console_CommandHistory(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    int i;

    (void)arg_int;
    (void)arg_float;
    if (narg == 0) {
        int number = 16;
        for (i = 0; number > 0; ++i, --number) {
            char *line = console_history[(console_historypos + i) & 15];
            if (strlen(line) != 0) {
                console_Printf("%2d: %s\n", number, line);
            }
        }
        return 1;
    }
    if (_stricmp(arg_str[0], "clear") == 0) {
        for (i = 0; i < CONSOLE_HISTORY_LINES; ++i) {
            console_history[i][0] = '\0';
        }
        console_historypos = 0;
        console_historybrowser = 0;
        console_Printf("History cleared\n");
        return 1;
    }
    console_Printf("HISTORY - show/clear the command history\n");
    console_Printf("usage: history [clear]\n");
    return 1;
}

/* 0x28B00, 657 bytes */
int console_CommandKey(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    ConsoleKeyBinding *binding;
    unsigned char key;
    int i;

    (void)arg_int;
    (void)arg_float;
    if (narg == 1) {
        if (arg_str[0][0] == '?') {
            console_Printf("KEY - assign hotkeys...\n");
            console_Printf(
                "usage: key [[key] [command1 ... commandN|remove]]\n");
            console_Printf(
                "to remove a binding, key [key] remove\n");
            return 0;
        }
        key = findkey(arg_str[0]);
        if (key == 0) {
            console_Printf("KEY %s not recognized\n", arg_str[0]);
            return 0;
        }
        binding = findbinding(key);
        if (binding != NULL) {
            console_Printf(
                "KEY %s:\n[%s]\n", arg_str[0], binding->commandline);
        }
        return 0;
    }
    if (narg == 0) {
        int total = 0;

        console_Printf("KEY BINDINGS:\n");
        for (i = 0; i < CONSOLE_BINDINGS; ++i) {
            char *key_name = NULL;
            int name_index;

            if (console_keybinding[i].key == 0) {
                continue;
            }
            for (name_index = 0;
                 console_keynames[name_index] != NULL;
                 ++name_index) {
                if ((unsigned char)console_keynames[name_index][0] ==
                    console_keybinding[i].key) {
                    key_name = console_keynames[name_index] + 1;
                    break;
                }
            }
            if (key_name == NULL) {
                console_name[0] = (char)console_keybinding[i].key;
                console_name[1] = '\0';
                key_name = console_name;
            }
            console_Printf(
                "%2d %8s: %s\n",
                total,
                key_name,
                console_keybinding[i].commandline);
            ++total;
        }
        console_Printf("%d keys bound\n", total);
        return 0;
    }

    key = findkey(arg_str[0]);
    if (key == 0) {
        console_Printf("KEY %s not recognized\n", arg_str[0]);
        return 0;
    }
    binding = findbinding(key);
    if (_stricmp(arg_str[1], "remove") == 0) {
        if (binding != NULL) {
            binding->key = 0;
            console_Printf("KEY %s un-bound\n", arg_str[0]);
        } else {
            console_Printf("KEY %s not bound!?\n", arg_str[0]);
        }
        return 0;
    }
    if (binding == NULL) {
        for (i = 0; i < CONSOLE_BINDINGS; ++i) {
            if (console_keybinding[i].key == 0) {
                binding = &console_keybinding[i];
                break;
            }
        }
        if (binding == NULL) {
            console_Printf("Too many key bindings - remove some!\n");
            return 0;
        }
    }
    binding->commandline[0] = '\0';
    for (i = 1; i < narg; ++i) {
        if (i > 1) {
            strncat(binding->commandline, " ", 255);
        }
        strncat(binding->commandline, arg_str[i], 255);
    }
    binding->key = key;
    return 0;
}

/* 0x28DA0, 97 bytes */
int console_CommandList(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    int from = 0;
    int to = 0x7fffffff;

    (void)arg_float;
    if (narg < 3 && (narg != 1 || arg_str[0][0] != '?')) {
        if (narg > 0) {
            from = arg_int[0];
        }
        if (narg == 2) {
            to = arg_int[1];
        }
        basic_List(from, to);
        return 1;
    }
    console_Printf("LIST - list the program\n");
    console_Printf("usage: list [start [end]]\n");
    return 1;
}

/* 0x28E10, 428 bytes */
int console_CommandLoadKeys(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    unsigned char *buffer;
    unsigned long size;
    int result;
    int i;

    (void)arg_int;
    (void)arg_float;
    if (narg < 2 && (narg != 1 || arg_str[0][0] != '?')) {
        ConsoleInfoHeader *info;
        ConsoleKeyBinding *saved_bindings;

        memcard_on();
        result = memcard_LoadFile(0, "JPBKEYS.JPB", &buffer, &size);
        memcard_off();
        if (result != 0) {
            console_Printf("ERROR %d loading %s\n", result, arg_str[0]);
            return 1;
        }
        info = (ConsoleInfoHeader *)buffer;
        console_alpha = info->console_alpha;
        console_ylines = info->console_ylines;
        console_Printf(
            "Alpha: %d\nYLines: %d\nKeys: %d\n",
            console_alpha,
            console_ylines,
            info->numkeybindings);
        saved_bindings = (ConsoleKeyBinding *)(buffer + sizeof(*info));
        i = 0;
        if ((unsigned)(info->numkeybindings - 1) < 63) {
            for (; i < info->numkeybindings; ++i) {
                memcpy(
                    &console_keybinding[i],
                    &saved_bindings[i],
                    sizeof(console_keybinding[i]));
            }
        }
        for (; i < CONSOLE_BINDINGS; ++i) {
            console_keybinding[i].key = 0;
        }
        free(buffer);
        return 1;
    }
    console_Printf(
        "LOADKEYS - load keybindings from memcard file JPBKEYS.JPB\n");
    console_Printf("usage - loadkeys\n");
    return 1;
}

/* 0x28FC0, 60 bytes */
int console_CommandLoadProgram(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] != '?') {
        basic_LoadProgram(arg_str[0], console_currentmemcard);
    } else {
        console_Printf("LOAD - load a program from memcard\n");
        console_Printf("usage: load [filename]\n");
    }
}

/* 0x29000, 54 bytes */
int console_CommandNew(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] == '?') {
        console_Printf("NEW - delete current program\n");
        console_Printf("usage: new\n");
    } else {
        basic_New();
    }
}

/* 0x29040, 61 bytes */
int console_CommandRun(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    int result;

    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] == '?') {
        console_Printf("RUN - run the program\n");
        console_Printf("usage: run\n");
    }
    result = basic_PrepareToRun();
    console_basicrunning = 1;
    return result;
}

/* 0x29080, 1186 bytes */
int console_CommandSaveKeys(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    ConsoleInfoHeader *info;
    ConsoleKeyBinding *saved_bindings;
    int count = 0;
    int i;
    unsigned long size;

    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] == '?') {
        console_Printf(
            "SAVEKEYS - save keybindings to memcard file JPBKEYS.JPB\n");
        console_Printf("usage - savekeys\n");
        return 1;
    }
    while (count < CONSOLE_BINDINGS &&
           console_keybinding[count].key != 0) {
        ++count;
    }
    size = (unsigned long)(sizeof(*info) +
                           count * sizeof(ConsoleKeyBinding));
    info = (ConsoleInfoHeader *)malloc(size);
    if (info != NULL) {
        info->console_alpha = console_alpha;
        info->console_ylines = console_ylines;
        info->numkeybindings = count;
        saved_bindings = (ConsoleKeyBinding *)
            ((unsigned char *)info + sizeof(*info));
        for (i = 0; i < count; ++i) {
            if (console_keybinding[i].key != 0) {
                memcpy(
                    &saved_bindings[i],
                    &console_keybinding[i],
                    sizeof(saved_bindings[i]));
            }
        }
        memcard_on();
        memcard_SaveFile(
            0, "JPBKEYS.JPB", (unsigned char *)info, size);
        memcard_off();
        console_Printf("Saved console info to %s\n", "JPBKEYS.JPB");
        free(info);
    }
    return 1;
}

/* 0x29530, 60 bytes */
int console_CommandSaveProgram(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)arg_int;
    (void)arg_float;
    if (narg == 1 && arg_str[0][0] != '?') {
        basic_SaveProgram(arg_str[0], console_currentmemcard);
    } else {
        console_Printf("SAVE - save a program to memcard\n");
        console_Printf("usage: save [filename]\n");
    }
}

/* 0x29570, 358 bytes */
void console_Display(void)
{
    int i;

    if (bConsoleActive == 0 || debug_slomo != 0) {
        return;
    }
    if (console_yscroll < console_yscrolldest) {
        ++console_yscroll;
    } else if (console_yscroll > console_yscrolldest) {
        --console_yscroll;
    }
    for (i = 0; i < console_ylines; ++i) {
        console_text(
            20.0f,
            (float)((console_ylines - i - 1) * 16),
            ((unsigned long)console_alpha << 24) | console_color[0],
            consolebuffer[(console_y - console_yscroll - i) & 255]);
    }
    ++cursortimer;
    if ((cursortimer & 8) == 0 && console_yscroll == 0) {
        static char cursor[] = "\x80";
        console_text(
            (float)console_x * 8.0f + 20.0f,
            (float)(console_ylines * 16 - 16),
            ((unsigned long)console_alpha << 24) | console_color[2],
            cursor);
    }
    console_rectangle(
        0.0f,
        0.0f,
        640.0f,
        (float)(console_ylines << 4),
        (long)(((unsigned long)console_alpha << 24) | console_color[1]));
}

/* 0x296E0, 43 bytes */
int console_EnableConsoleCommand(
    int narg, char **arg_str, int *arg_int, float *arg_float)
{
    (void)narg;
    (void)arg_str;
    (void)arg_int;
    (void)arg_float;
    console_Printf("Welcome to Jedi Power Basic!\n");
    console_Printf("Type 'help' for help!\n");
    console_enabled = 1;
}

/* 0x29710, 211 bytes */
void console_Execute(int startline, int instant)
{
    commandbuffer[0] = '\0';
    do {
        strncat(commandbuffer, consolebuffer[startline], 256);
        startline = (startline + 1) % 256;
    } while ((unsigned)startline != console_y);

    if (strlen(commandbuffer + 1) != 0) {
        if (instant == 0) {
            strncpy(
                console_history[console_historypos],
                commandbuffer + 1,
                256);
            console_historypos = (console_historypos + 1) & 15;
            console_historybrowser = 0;
        }
        console_ProcessCommandLine(commandbuffer + 1);
    }
}

/* 0x297F0, 1581 bytes */
void console_HandleConsole(void)
{
    unsigned char key = (unsigned char)LastKey();

    if (bKeyboardNabbed != 0) {
        return;
    }
    if (bInited == 0) {
        console_Init();
        bInited = 1;
        console_Cls();
    }
    if (key == '`') {
        bConsoleActive = bConsoleActive == 0;
    }

    if (console_basicrunning == 0) {
        if (bConsoleActive == 0 || key == '`') {
            if (bKeyboardNabbed == 0 && key != 0) {
                int i;
                for (i = 0; i < CONSOLE_BINDINGS; ++i) {
                    if (console_keybinding[i].key == key) {
                        char command[256];
                        strncpy(
                            command,
                            console_keybinding[i].commandline,
                            255);
                        console_ProcessCommandLine(command);
                        break;
                    }
                }
            }
            return;
        }
        if (key != 0) {
            char string[16];
            string[0] = (char)key;
            string[1] = '\0';

            switch (key) {
            case 8:
                console_BackSpace();
                break;
            case 10:
            case 13:
                console_historybrowser = 0;
                console_AddString(string);
                console_Execute((int)console_commandline, 0);
                if (console_basicrunning == 0) {
                    console_AddString("]");
                    console_commandline = console_y;
                }
                break;
            case 27:
                console_ClearCommandLine();
                console_historybrowser = 0;
                break;
            case 0x1c:
                if (ShiftKeyDown() != 0) {
                    console_alpha -= 16;
                    if (console_alpha < 0) {
                        console_alpha = 0;
                    }
                }
                break;
            case 0x1d:
                if (ShiftKeyDown() != 0) {
                    console_alpha += 16;
                    if (console_alpha > 255) {
                        console_alpha = 255;
                    }
                }
                break;
            case 0x1e:
                if (ShiftKeyDown() == 0) {
                    if (console_historybrowser < 16) {
                        unsigned index =
                            (console_historypos -
                             console_historybrowser - 1) & 15;
                        if (strlen(console_history[index]) != 0) {
                            ++console_historybrowser;
                            console_ClearCommandLine();
                            console_AddString(console_history[
                                (console_historypos -
                                 console_historybrowser) & 15]);
                        }
                    }
                } else {
                    --console_ylines;
                    if (console_ylines < 2) {
                        console_ylines = 2;
                    }
                }
                break;
            case 0x1f:
                if (ShiftKeyDown() == 0) {
                    if (console_historybrowser > 1) {
                        --console_historybrowser;
                        console_ClearCommandLine();
                        console_AddString(console_history[
                            (console_historypos -
                             console_historybrowser) & 15]);
                    } else {
                        console_historybrowser = 0;
                        console_ClearCommandLine();
                    }
                } else {
                    ++console_ylines;
                    if (console_ylines > 65) {
                        console_ylines = 65;
                    }
                }
                break;
            case 0x95:
                console_yscrolldest = 256 - console_ylines;
                break;
            case 0x96:
                console_yscrolldest = 0;
                break;
            case 0x97:
                if (abs(console_yscroll - console_yscrolldest) <
                    console_ylines * 2) {
                    console_yscrolldest += console_ylines - 2;
                    if (console_yscrolldest > 256 - console_ylines) {
                        console_yscrolldest = 256 - console_ylines;
                    }
                }
                break;
            case 0x98:
                if (abs(console_yscroll - console_yscrolldest) <
                    console_ylines * 2) {
                    console_yscrolldest += 2 - console_ylines;
                    if (console_yscrolldest < 0) {
                        console_yscrolldest = 0;
                    }
                }
                break;
            default:
                console_yscrolldest = 0;
                console_AddString(string);
                break;
            }
            cursortimer = 0;
        }
        console_Display();
        return;
    }

    if (key == 27) {
        console_basicrunning = 0;
        console_Printf("***break***\n");
    } else {
        console_basicrunning = basic_Run(0);
    }
    if (console_basicrunning == 0) {
        console_commandline = console_y;
        console_AddString("]");
        console_Display();
        return;
    }
    console_Display();
}

/* 0x29E20, 11 bytes */
void console_HideConsole(void)
{
    bConsoleActive = 0;
}

/* 0x29E30, 2092 bytes */
static void console_Init(void)
{
    int i;

    for (i = 0; i < CONSOLE_LINES; ++i) {
        consolebuffer[i][0] = '\0';
    }
    ++console_y;
    console_alpha = 176;
    console_y = (unsigned char)console_y;
    console_commandline = console_y;
    consolebuffer[console_y][0] = '\0';
    console_x = 0;
    console_AddString("]");
    for (i = 0; i < CONSOLE_HISTORY_LINES; ++i) {
        console_history[i][0] = '\0';
    }
    console_historypos = 0;
    console_historybrowser = 0;
    console_numcommands = 0;
    console_yscroll = 0;
    console_yscrolldest = 0;
    console_ylines = 24;
    memset(console_keybinding, 0, sizeof(console_keybinding));
    basic_Init();

    console_AddCommand("help", "help", console_CommandHelp);
    console_AddCommand("cls", "cls", console_CommandCls);
    console_AddCommand("color", "col", console_CommandColor);
    console_AddCommand("dump", "dump", console_CommandDump);
    console_AddCommand("history", "hist", console_CommandHistory);
    console_AddCommand("key", "k", console_CommandKey);
    console_AddCommand("card", "cd", console_CommandCard);
    console_AddCommand("savekeys", "sk", console_CommandSaveKeys);
    console_AddCommand("loadkeys", "lk", console_CommandLoadKeys);
    console_AddCommand("dir", "dir", console_CommandDir);
    console_AddCommand("list", "list", console_CommandList);
    console_AddCommand("run", "run", console_CommandRun);
    console_AddCommand("new", "new", console_CommandNew);
    console_AddCommand("save", "save", console_CommandSaveProgram);
    console_AddCommand("load", "load", console_CommandLoadProgram);
    console_AddCommand("exit", "x", console_CommandExit);
    console_AddCommand("power", "pwr", console_PowerCommand);
}

/* 0x2A660, 28 bytes */
int console_NabKeyboard(void)
{
    if (bKeyboardNabbed != 0) {
        return 0;
    }
    bKeyboardNabbed = 1;
    return 1;
}

/* 0x2A680, 81 bytes */
int console_NextLine(char c)
{
    console_x = 0;
    console_y = (unsigned char)(console_y + 1);
    if (c != '\0') {
        consolebuffer[console_y][0] = c;
        console_x = 1;
    }
    consolebuffer[console_y][console_x] = '\0';
    return 0;
}

/* 0x2A6E0, 147 bytes */
int console_Printf(char *format, ...)
{
    char buffer[256];
    va_list arguments;

    va_start(arguments, format);
    vsprintf(buffer, format, arguments);
    va_end(arguments);
    console_AddString(buffer);
}

/* 0x2A780, 617 bytes */
int console_ProcessCommandLine(char *cmd)
{
    int line_number = atoi(cmd);
    int mode;
    char *token = NULL;
    signed char argument_count = 0;
    int i;

    if (line_number != 0 && console_basicrunning == 0) {
        basic_AddLine(cmd);
    } else {
        mode = 0;
        while (mode != -1 && argument_count < 16) {
            char c = *cmd;

            if (mode == 0) {
                if (c == '\0') {
                    mode = -1;
                } else if (c != ' ') {
                    if (c == '"') {
                        mode = 1;
                        token = cmd + 1;
                    } else {
                        mode = 2;
                        token = cmd;
                    }
                }
            } else if (mode == 1) {
                if (c == '\0' || c == '"') {
                    commandargs[argument_count++] = token;
                    *cmd = '\0';
                    mode = c == '\0' ? -1 : 0;
                }
            } else if (mode == 2 && (c == '\0' || c == ' ')) {
                commandargs[argument_count++] = token;
                *cmd = '\0';
                mode = c == '\0' ? -1 : 0;
            }
            ++cmd;
        }

        for (i = 0; i < console_numcommands; ++i) {
            if (_stricmp(commandargs[0], console_command[i].name) == 0 ||
                _stricmp(commandargs[0], console_command[i].shortname) == 0) {
                int argument;
                for (argument = 1;
                     argument < argument_count;
                     ++argument) {
                    commandfloat[argument - 1] =
                        (float)atof(commandargs[argument]);
                    commandint[argument - 1] =
                        atoi(commandargs[argument]);
                }
                if (console_command[i].handler ==
                    console_EnableConsoleCommand) {
                    console_enabled = 1;
                }
                if (console_enabled != 0) {
                    return console_command[i].handler(
                        argument_count - 1,
                        &commandargs[1],
                        commandint,
                        commandfloat);
                }
            }
        }
        return console_Printf(
            "[%s] unknown command\n", commandargs[0]);
    }
}

/* 0x2A9F0, 13 bytes */
int console_ReleaseKeyboard(void)
{
    bKeyboardNabbed = 0;
    return 0;
}

/* 0x2AA00, 173 bytes */
void console_ResetCommandLine(void)
{
    console_AddString("]");
    console_commandline = console_y;
}

/* 0x2AAB0, 11 bytes */
void console_ShowConsole(void)
{
    bConsoleActive = 1;
}

/* 0x2AAC0, 54 bytes */
static ConsoleKeyBinding *findbinding(unsigned char key)
{
    int i;
    for (i = 0; i < CONSOLE_BINDINGS; ++i) {
        if (console_keybinding[i].key == key) {
            return &console_keybinding[i];
        }
    }
    return NULL;
}

/* 0x2AB00, 146 bytes */
static unsigned char findkey(char *string)
{
    int i;

    if (strlen(string) == 1) {
        return (unsigned char)string[0];
    }
    for (i = 0; console_keynames[i] != NULL; ++i) {
        if (_stricmp(string, console_keynames[i] + 1) == 0) {
            return (unsigned char)console_keynames[i][0];
        }
    }
    return 0;
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

const char *jpb_ConsoleBufferLine(size_t index)
{
    return index < CONSOLE_LINES ? consolebuffer[index] : NULL;
}

int jpb_ConsoleBufferColumn(void)
{
    return (int)console_x;
}

int jpb_ConsoleBufferRow(void)
{
    return (int)console_y;
}

int jpb_ConsoleScroll(void)
{
    return console_yscroll;
}

int jpb_ConsoleScrollDestination(void)
{
    return console_yscrolldest;
}

int jpb_ConsoleVisibleLines(void)
{
    return console_ylines;
}

int jpb_ConsoleAlpha(void)
{
    return console_alpha;
}

const ConsoleKeyBinding *jpb_ConsoleKeyBinding(size_t index)
{
    return index < CONSOLE_BINDINGS ? &console_keybinding[index] : NULL;
}
