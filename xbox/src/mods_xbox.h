#ifndef JPB_XBOX_MODS_H
#define JPB_XBOX_MODS_H
#include <stddef.h>

/* Version of the converted package contents this build reads. PC packages
   are converted by tools/stage_mods.py, which stamps xbox-package.json with
   the format it wrote. Raise this, and the tool's XBOX_FORMAT, whenever the
   Xbox representation of package files changes; older conversions are then
   rejected with a request to reconvert rather than misread. */
#define JPB_XBOX_MOD_FORMAT 3

/* Every D:\mods\<id> with a mod.json must carry a matching conversion stamp.
   Returns 0 with a reason otherwise. */
int jpb_XboxModsCheckFormat(char *error, size_t capacity);
/* Calls visit with each package directory (D:\mods\<id>). */
void jpb_XboxModsForEachPackage(void (*visit)(const char *directory, void *user),
    void *user);
#endif
