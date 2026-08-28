#ifndef JPB_CONSOLE_H
#define JPB_CONSOLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ConsoleCommandHandler)(
    int argument_count,
    char **string_arguments,
    int *integer_arguments,
    float *float_arguments);

typedef struct ConsoleCommand {
    char *name;
    char *shortname;
    ConsoleCommandHandler handler;
} ConsoleCommand;

typedef struct ConsoleKeyBinding {
    char commandline[255];
    unsigned char key;
} ConsoleKeyBinding;

extern int console_currentmemcard;
extern int bConsoleActive;
extern int console_basicrunning;
extern int bKeyboardNabbed;
extern int console_enabled;

unsigned long ascii2hex(char *string);

int console_AddCommand(
    char *name,
    char *shortname,
    ConsoleCommandHandler handler);
void console_AddString(char *string);
void console_BackSpace(void);
void console_ClearCommandLine(void);
void console_Cls(void);
int console_CommandCard(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandCls(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandColor(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandDelete(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandDir(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandDump(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandExit(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandHelp(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandHistory(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandKey(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandList(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandLoadKeys(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandLoadProgram(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandNew(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandRun(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandSaveKeys(
    int argument_count, char **strings, int *integers, float *floats);
int console_CommandSaveProgram(
    int argument_count, char **strings, int *integers, float *floats);
void console_Display(void);
int console_EnableConsoleCommand(
    int argument_count, char **strings, int *integers, float *floats);
void console_Execute(int start_line, int instant);
void console_HandleConsole(void);
void console_HideConsole(void);
int console_NabKeyboard(void);
int console_NextLine(char first_character);
int console_Printf(char *format, ...);
int console_ProcessCommandLine(char *command);
int console_ReleaseKeyboard(void);
void console_ResetCommandLine(void);
void console_ShowConsole(void);

/* Passive inspection/reset seams for the 256-slot, 255-usable registry. */
void jpb_ConsoleResetCommands(void);
size_t jpb_ConsoleCommandCount(void);
const char *jpb_ConsoleCommandName(size_t index);
const char *jpb_ConsoleCommandShortName(size_t index);
ConsoleCommandHandler jpb_ConsoleCommandHandler(size_t index);
const char *jpb_ConsoleBufferLine(size_t index);
int jpb_ConsoleBufferColumn(void);
int jpb_ConsoleBufferRow(void);
int jpb_ConsoleScroll(void);
int jpb_ConsoleScrollDestination(void);
int jpb_ConsoleVisibleLines(void);
int jpb_ConsoleAlpha(void);
const ConsoleKeyBinding *jpb_ConsoleKeyBinding(size_t index);

#ifdef __cplusplus
}
#endif

#endif
