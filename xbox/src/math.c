/* The installed nxdk PDCLIB lroundf is an assert-only stub. Animation
   blending needs C's nearest-integer, halfway-away-from-zero semantics. */
#include <math.h>
#include <limits.h>
#include <errno.h>

long jpb_XboxRoundLong(float value)
{
    double rounded = value < 0 ? ceil((double)value - 0.5) : floor((double)value + 0.5);
    if (!(rounded >= (double)LONG_MIN && rounded <= (double)LONG_MAX)) {
        errno = EDOM;
        return LONG_MIN;
    }
    return (long)rounded;
}

#if defined(JPB_XBOX)
long lroundf(float value) { return jpb_XboxRoundLong(value); }
#endif
