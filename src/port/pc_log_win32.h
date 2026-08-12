#ifndef JPB_PC_LOG_WIN32_H
#define JPB_PC_LOG_WIN32_H

#include <stddef.h>

void jpb_PCLogStart(int argc, char **argv);
void jpb_PCLog(const char *format, ...);
void jpb_PCLogSetCheckpoint(const char *format, ...);
size_t jpb_PCLogFormatException(
    void *exception_pointers, char *buffer, size_t capacity);
void jpb_PCLogException(void *exception_pointers);
void jpb_PCLogStop(int exit_code);
const char *jpb_PCLogPath(void);

#endif
