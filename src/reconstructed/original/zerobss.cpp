/*
 * REVIEWED RECONSTRUCTION of W:\SWJediPowerBattles\work\zerobss.cpp.
 * PDB module: 0105
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\zerobss.obj
 * Primary source: W:\SWJediPowerBattles\work\zerobss.cpp
 * Compiler language: c++
 * Emitted project procedures: 2
 */

#include "jpb/zerobss.h"

#include "jpb/physics.h"
#include "jpb/player.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>

static std::array<void *, NUM_VARS> zeroBSSArray;

static_assert(sizeof(_svector) == 8, "_svector PDB layout changed");
static_assert(sizeof(_collide_info) == 52,
              "_collide_info PDB layout changed");
static_assert(sizeof(playerObject) == 568,
              "playerObject PDB layout changed");

static void *allocate_zeroed(std::size_t element_size, int size)
{
    const std::size_t count =
        size < 2 ? 1 : static_cast<std::size_t>(size);

    if (count > std::numeric_limits<std::size_t>::max() / element_size) {
        return nullptr;
    }

    const std::size_t byte_count = element_size * count;
    void *memory = ::operator new(byte_count, std::nothrow);
    if (memory != nullptr) {
        std::memset(memory, 0, byte_count);
    }
    return memory;
}

/* 0x12EC70, 956 bytes, global, 3 named locals
 * ZeroBSS
 * PDB type: void* (zerobssVars, zerobssType, int)
 * Source: W:\SWJediPowerBattles\work\zerobss.cpp
 */
void *ZeroBSS(zerobssVars var, zerobssType type, int size)
{
    void *&cached = zeroBSSArray[static_cast<std::size_t>(var)];
    std::size_t element_size;

    if (cached != nullptr) {
        return cached;
    }

    switch (type) {
    case zerobssINT:
    case zerobssLONG:
    case zerobssULONG:
    case zerobssFLOAT:
        element_size = 4;
        break;
    case zerobssCHAR:
        element_size = 1;
        break;
    case zerobssSHORT:
    case zerobssSOUNDHANDLE:
        element_size = 2;
        break;
    case zerobssSVECTOR:
        element_size = sizeof(_svector);
        break;
    case zerobssSPRITEPtr:
        element_size = sizeof(void *);
        break;
    case zerobssCOLLIDE_INFO:
        element_size = sizeof(_collide_info);
        break;
    case zerobssplayerObject:
        element_size = sizeof(playerObject);
        break;
    default:
        return nullptr;
    }

    cached = allocate_zeroed(element_size, size);
    return cached;
}

/* 0x12F030, 3 bytes, global, 0 named locals
 * ZeroBSS_ClearAll
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\zerobss.cpp
 */
void ZeroBSS_ClearAll(void)
{
}
