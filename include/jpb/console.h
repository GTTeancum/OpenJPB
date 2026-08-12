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

int console_AddCommand(
    char *name,
    char *shortname,
    ConsoleCommandHandler handler);

/* Descriptive inspection/reset seams for the exact 255-entry registry. */
void jpb_ConsoleResetCommands(void);
size_t jpb_ConsoleCommandCount(void);
const char *jpb_ConsoleCommandName(size_t index);
const char *jpb_ConsoleCommandShortName(size_t index);
ConsoleCommandHandler jpb_ConsoleCommandHandler(size_t index);

#ifdef __cplusplus
}
#endif

#endif
