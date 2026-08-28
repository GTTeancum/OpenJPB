/*
 * COMPLETE REVIEWED split reconstruction of debug_printf from
 * W:\SWJediPowerBattles\Work\wHook.cpp.
 *
 * Keeping this inert release procedure in its own archive member prevents
 * dependency-light file I/O callers from pulling the full renderer object.
 */

#include "jpb/debugtext.h"

/* Reference RVA 0x127B20, 18 bytes. */
int debug_printf(char *format, ...)
{
    (void)format;
    return 0;
}
