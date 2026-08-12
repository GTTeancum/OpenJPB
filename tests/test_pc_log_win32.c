#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "pc_log_win32.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(                                                       \
                stderr, "FAIL %s:%d: %s\n",                              \
                __FILE__, __LINE__, #condition);                           \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

int main(void)
{
    EXCEPTION_RECORD record;
    CONTEXT context;
    EXCEPTION_POINTERS pointers;
    uintptr_t module_base =
        (uintptr_t)(void *)GetModuleHandleA(NULL);
    char message[4096];
    size_t length;

    memset(&record, 0, sizeof(record));
    memset(&context, 0, sizeof(context));
    record.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
    record.ExceptionAddress = (void *)(module_base + UINT32_C(0x1234));
#if defined(_M_X64) || defined(__x86_64__)
    context.Rip = UINT64_C(0x1111222233334444);
    context.Rsp = UINT64_C(0x5555666677778888);
    context.Rbp = UINT64_C(0x9999aaaabbbbcccc);
#endif
    pointers.ExceptionRecord = &record;
    pointers.ContextRecord = &context;

    jpb_PCLogSetCheckpoint(
        "gameplay handoff p1 combos initialized model=8");
    length = jpb_PCLogFormatException(
        &pointers, message, sizeof(message));
    CHECK(length == strlen(message));
    CHECK(strstr(message, "code=0xc0000005") != NULL);
    CHECK(strstr(message, "rva=0x1234") != NULL);
    CHECK(strstr(
              message,
              "checkpoint=gameplay handoff p1 combos initialized model=8") !=
          NULL);
#if defined(_M_X64) || defined(__x86_64__)
    CHECK(strstr(message, "rip=0x1111222233334444") != NULL);
    CHECK(strstr(message, "rsp=0x5555666677778888") != NULL);
    CHECK(strstr(message, "rbp=0x9999aaaabbbbcccc") != NULL);
#endif

    length = jpb_PCLogFormatException(NULL, message, sizeof(message));
    CHECK(length == strlen(message));
    CHECK(strstr(message, "code=0x00000000") != NULL);
    CHECK(strstr(message, "checkpoint=gameplay handoff") != NULL);
    CHECK(jpb_PCLogFormatException(&pointers, NULL, 0) == 0);

    if (failures != 0) {
        fprintf(stderr, "%d PC log test(s) failed\n", failures);
        return 1;
    }
    puts("PC log tests passed");
    return 0;
}
