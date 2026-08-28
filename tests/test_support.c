#include "jpb/support.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    int first;
    int second;
    int expected;

    srand(17);
    CHECK(Random(0) == 0);
    first = rand();
    srand(17);
    CHECK(Random(13) == first % 13);
    srand(17);
    CHECK(Random(-13) == first % -13);

    srand(29);
    first = rand();
    second = rand();
    expected =
        ((first + 1) / (32767 / 16)) * 4096 - 4096 +
        (second + 1) / 7;
    srand(29);
    CHECK(f12Random(16) == expected);

    puts("support RNG tests passed");
    return 0;
}
