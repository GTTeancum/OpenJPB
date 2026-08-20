/* Persistent diagnostics for the dependency-free Win32 host. */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "pc_log_win32.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    PC_LOG_BUFFER_SIZE = 4096,
    PC_LOG_CHECKPOINT_SIZE = 1024
};

static HANDLE pc_log_file = INVALID_HANDLE_VALUE;
static char pc_log_path[MAX_PATH];
static char pc_log_checkpoint[PC_LOG_CHECKPOINT_SIZE] = "process startup";

static void pc_log_write(const char *text)
{
    DWORD written;
    size_t length;

    if (pc_log_file == INVALID_HANDLE_VALUE || text == NULL) {
        return;
    }
    length = strlen(text);
    while (length != 0) {
        DWORD chunk = length > MAXDWORD ? MAXDWORD : (DWORD)length;

        if (!WriteFile(pc_log_file, text, chunk, &written, NULL) ||
            written == 0) {
            break;
        }
        text += written;
        length -= written;
    }
}

static LONG WINAPI pc_log_unhandled_exception(
    EXCEPTION_POINTERS *exception)
{
    char message[PC_LOG_BUFFER_SIZE];

    (void)jpb_PCLogFormatException(
        exception, message, sizeof(message));
    pc_log_write(message);
    if (pc_log_file != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(pc_log_file);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

size_t jpb_PCLogFormatException(
    void *exception_pointers, char *buffer, size_t capacity)
{
    EXCEPTION_POINTERS *exception =
        (EXCEPTION_POINTERS *)exception_pointers;
    EXCEPTION_RECORD *record = exception != NULL
        ? exception->ExceptionRecord
        : NULL;
    CONTEXT *context = exception != NULL
        ? exception->ContextRecord
        : NULL;
    unsigned long code = record != NULL
        ? record->ExceptionCode
        : 0;
    const void *address = record != NULL
        ? record->ExceptionAddress
        : NULL;
    uintptr_t module_base =
        (uintptr_t)(void *)GetModuleHandleA(NULL);
    uintptr_t exception_address = (uintptr_t)address;
    size_t module_rva =
        exception_address >= module_base
            ? (size_t)(exception_address - module_base)
            : 0;
    int result;

    if (buffer == NULL || capacity == 0) {
        return 0;
    }

#if defined(_M_X64) || defined(__x86_64__)
    result = snprintf(
        buffer,
        capacity,
        "UNHANDLED EXCEPTION code=0x%08lx address=%p rva=0x%zx "
        "rip=0x%llx rsp=0x%llx rbp=0x%llx thread=%lu "
        "checkpoint=%s\r\n",
        code,
        address,
        module_rva,
        context != NULL ? (unsigned long long)context->Rip : 0,
        context != NULL ? (unsigned long long)context->Rsp : 0,
        context != NULL ? (unsigned long long)context->Rbp : 0,
        (unsigned long)GetCurrentThreadId(),
        pc_log_checkpoint);
#else
    result = snprintf(
        buffer,
        capacity,
        "UNHANDLED EXCEPTION code=0x%08lx address=%p rva=0x%zx "
        "thread=%lu checkpoint=%s\r\n",
        code,
        address,
        module_rva,
        (unsigned long)GetCurrentThreadId(),
        pc_log_checkpoint);
#endif
    buffer[capacity - 1] = '\0';
    if (result < 0) {
        buffer[0] = '\0';
        return 0;
    }
    return strlen(buffer);
}

void jpb_PCLogException(void *exception_pointers)
{
    (void)pc_log_unhandled_exception(
        (EXCEPTION_POINTERS *)exception_pointers);
}

void jpb_PCLogStart(int argc, char **argv)
{
    char executable[MAX_PATH];
    char *separator;
    int index;

    pc_log_path[0] = '\0';
    /* Automated/headless callers already retain stdout and stderr. Keeping
     * them out of the shared interactive log also prevents parallel test
     * processes from truncating one another's diagnostics. */
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--headless") == 0) {
            return;
        }
    }
    if (GetModuleFileNameA(NULL, executable, MAX_PATH) == 0) {
        return;
    }
    executable[MAX_PATH - 1] = '\0';
    separator = strrchr(executable, '\\');
    if (separator == NULL) {
        separator = strrchr(executable, '/');
    }
    if (separator != NULL) {
        separator[1] = '\0';
    } else {
        executable[0] = '\0';
    }
    if (snprintf(
            pc_log_path,
            sizeof(pc_log_path),
            "%sjpb_pc_game.log",
            executable) < 0) {
        pc_log_path[0] = '\0';
        return;
    }
    pc_log_file = CreateFileA(
        pc_log_path,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (pc_log_file == INVALID_HANDLE_VALUE) {
        pc_log_path[0] = '\0';
        return;
    }
    SetUnhandledExceptionFilter(pc_log_unhandled_exception);
    jpb_PCLog("log started pid=%lu", (unsigned long)GetCurrentProcessId());
    for (index = 0; index < argc; ++index) {
        jpb_PCLog("argv[%d]=%s", index, argv[index]);
    }
}

void jpb_PCLog(const char *format, ...)
{
    SYSTEMTIME time;
    char message[PC_LOG_BUFFER_SIZE];
    char line[PC_LOG_BUFFER_SIZE];
    va_list arguments;

    if (pc_log_file == INVALID_HANDLE_VALUE || format == NULL) {
        return;
    }
    va_start(arguments, format);
    (void)vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    GetLocalTime(&time);
    (void)snprintf(
        line,
        sizeof(line),
        "%04u-%02u-%02u %02u:%02u:%02u.%03u %s\r\n",
        (unsigned)time.wYear,
        (unsigned)time.wMonth,
        (unsigned)time.wDay,
        (unsigned)time.wHour,
        (unsigned)time.wMinute,
        (unsigned)time.wSecond,
        (unsigned)time.wMilliseconds,
        message);
    pc_log_write(line);
}

void jpb_PCLogSetCheckpoint(const char *format, ...)
{
    va_list arguments;

    if (format == NULL) {
        return;
    }
    va_start(arguments, format);
    (void)vsnprintf(
        pc_log_checkpoint,
        sizeof(pc_log_checkpoint),
        format,
        arguments);
    va_end(arguments);
    pc_log_checkpoint[sizeof(pc_log_checkpoint) - 1] = '\0';
}

void jpb_PCLogStop(int exit_code)
{
    if (pc_log_file == INVALID_HANDLE_VALUE) {
        return;
    }
    jpb_PCLog("clean shutdown exit=%d", exit_code);
    FlushFileBuffers(pc_log_file);
    CloseHandle(pc_log_file);
    pc_log_file = INVALID_HANDLE_VALUE;
}

const char *jpb_PCLogPath(void)
{
    return pc_log_path[0] != '\0' ? pc_log_path : NULL;
}
