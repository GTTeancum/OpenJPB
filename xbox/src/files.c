#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "jpb/mods.h"

/* The portable resource resolver emits forward slashes. Xbox object paths
   require backslashes; normalize only at this platform file boundary.
   Reads of installed resources (D:\res\...) go to an enabled mod package's
   copy when one provides that path, as on PC. */
FILE *jpb_XboxFopen(const char *path, const char *mode)
{
    char normalized[1024];
    char resolved[JPB_MOD_PATH];
    size_t i;
    if (!path || strlen(path) >= sizeof(normalized)) { errno = EINVAL; return NULL; }
    if (mode && mode[0] == 'r' &&
        jpb_ModsResolvePath(path, resolved, sizeof(resolved)))
        path = resolved;
    for (i = 0; path[i]; ++i) normalized[i] = path[i] == '/' ? '\\' : path[i];
    normalized[i] = 0;
    return fopen(normalized, mode);
}
