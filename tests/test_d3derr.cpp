#include "jpb/d3derr.h"

#include <cstdio>
#include <cstring>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "check failed at %s:%d: %s\n",            \
                         __FILE__, __LINE__, #condition);                    \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main()
{
    unsigned index;

    CHECK(sizeof(alldderrs) / sizeof(alldderrs[0]) == 199);
    for (index = 0; index < 198; ++index) {
        CHECK(alldderrs[index].err != 0);
        CHECK(dderrmsg(alldderrs[index].err) == alldderrs[index].errmsg);
    }
    CHECK(alldderrs[198].err == 0);
    CHECK(alldderrs[198].errmsg == nullptr);
    CHECK(std::strcmp(dderrmsg(0), "?Noerror?") == 0);
    CHECK(std::strcmp(dderrmsg(0xdeadbeefu), "?Noerror?") == 0);

    CHECK(std::strcmp(dderrmsg(0x88760005u), "Alreadyinitialized") == 0);
    CHECK(std::strcmp(dderrmsg(0x8007000eu), "Outofmemory") == 0);
    CHECK(std::strcmp(dderrmsg(0x887602c1u), "Invalid_device") == 0);
    CHECK(std::strcmp(dderrmsg(0x88760836u), "Notinbeginstateblock") == 0);

    char ignored[] = "ignored";
    char file[] = "retail.cpp";
    d3derr(0x88760005u, ignored, 73, file);

    std::puts("DirectDraw error table tests passed");
    return 0;
}
