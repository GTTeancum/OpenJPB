#include "jpb/zerobss.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "check failed at %s:%d: %s\n",          \
                         __FILE__, __LINE__, #condition);                    \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int check_zeroed(void *memory, std::size_t byte_count)
{
    const auto *bytes = static_cast<const std::uint8_t *>(memory);
    for (std::size_t index = 0; index < byte_count; ++index) {
        CHECK(bytes[index] == 0);
    }
    return 0;
}

static int check_all_types(void)
{
    struct TypeCase {
        zerobssType type;
        std::size_t element_size;
    } cases[] = {
        {zerobssINT, 4}, {zerobssCHAR, 1}, {zerobssSHORT, 2},
        {zerobssLONG, 4}, {zerobssULONG, 4}, {zerobssFLOAT, 4},
        {zerobssSVECTOR, 8}, {zerobssSPRITEPtr, 8},
        {zerobssSOUNDHANDLE, 2}, {zerobssCOLLIDE_INFO, 52},
        {zerobssplayerObject, 568},
    };

    for (std::size_t index = 0; index < std::size(cases); ++index) {
        const auto var = static_cast<zerobssVars>(index);
        void *memory = ZeroBSS(var, cases[index].type, 3);
        CHECK(memory != nullptr);
        CHECK(check_zeroed(memory, cases[index].element_size * 3) == 0);
    }
    return 0;
}

static int check_cache_contract(void)
{
    void *first = ZeroBSS(e_succeed, zerobssINT, 4);
    CHECK(first != nullptr);
    std::memset(first, 0x5a, 16);

    void *second = ZeroBSS(e_succeed, zerobssplayerObject, 20);
    CHECK(second == first);
    CHECK(static_cast<std::uint8_t *>(second)[0] == 0x5a);

    ZeroBSS_ClearAll();
    CHECK(ZeroBSS(e_succeed, zerobssCHAR, 1) == first);
    CHECK(static_cast<std::uint8_t *>(first)[15] == 0x5a);
    return 0;
}

static int check_edge_cases(void)
{
    void *single = ZeroBSS(e_failed, zerobssCOLLIDE_INFO, -9);
    CHECK(single != nullptr);
    CHECK(check_zeroed(single, 52) == 0);

    CHECK(ZeroBSS(e_delay, static_cast<zerobssType>(99), 3) == nullptr);
    void *retry = ZeroBSS(e_delay, zerobssSHORT, 3);
    CHECK(retry != nullptr);
    CHECK(check_zeroed(retry, 6) == 0);
    return 0;
}

int main()
{
    CHECK(check_all_types() == 0);
    CHECK(check_cache_contract() == 0);
    CHECK(check_edge_cases() == 0);
    std::puts("ZeroBSS tests passed");
    return 0;
}
