#include "jpb/win_memcard.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",              \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    unsigned char *data = (unsigned char *)(size_t)0x1234;
    unsigned long size = 0x5678;

    CHECK(strcmp(cardfile(7, "save.dat"),
                 "c:\\katavo\\winver\\memcard\\7\\save.dat") == 0);

    console_currentmemcard = 3;
    CHECK(strcmp(cardfile(-1, "slot.bin"),
                 "c:\\katavo\\winver\\memcard\\3\\slot.bin") == 0);

    CHECK(memcard_FileExists(2147483647, "jpb_missing_file") == 0);
    CHECK(memcard_LoadFile(2147483647, "jpb_missing_file", &data, &size)
          == -4);
    CHECK(data == (unsigned char *)(size_t)0x1234);
    CHECK(size == 0x5678);

    kmMemcard_Delete(0, NULL);
    kmMemcard_Exit();
    kmMemcard_Load(0, NULL);
    kmMemcard_Save(0, NULL, NULL, 0);
    kmMemcard_Update();
    memcard_off();
    memcard_on();

    puts("Win32 memory-card tests passed");
    return 0;
}
