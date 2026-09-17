#include <stdio.h>
#include <errno.h>
#include <string.h>

/* The portable resource resolver emits forward slashes. Xbox object paths
   require backslashes; normalize only at this platform file boundary. */
FILE *jpb_XboxFopen(const char *path, const char *mode)
{
    char normalized[1024];
    size_t i;
    if (!path || strlen(path) >= sizeof(normalized)) { errno = EINVAL; return NULL; }
    for (i = 0; path[i]; ++i) normalized[i] = path[i] == '/' ? '\\' : path[i];
    normalized[i] = 0;
    return fopen(normalized, mode);
}
