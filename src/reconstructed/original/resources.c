/*
 * REVIEWED RECONSTRUCTION of
 * W:\\SWJediPowerBattles\\work\\resources.c.
 *
 * The exact resource-type mapping and 256-byte path publication buffer are
 * preserved. SDL_GetBasePath and Sleep are isolated behind descriptive host
 * seams so the game-owned path logic remains dependency-light.
 * PDB module: 0072
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\resources.obj
 * Primary source: W:\SWJediPowerBattles\work\resources.c
 * Compiler language: c
 * Emitted procedures: 5
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/resources.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* Exact local PDB names at RVAs 0x546650 and 0x546660. */
static char basePath[JPB_RESOURCE_PATH_CAPACITY];
static char path[JPB_RESOURCE_PATH_CAPACITY];
static JPBResourceLoadingUpdateHook loadingUpdateHook;
static void *loadingUpdateUserData;

static const char *const resourceDirectories[
    JPB_RESOURCE_TYPE_COUNT] = {
    "sound/sfx/final/",
    "sound/streams/",
    "effects/tga/",
    "combo/",
    "effects/",
    "level/w3d/",
    "model/",
    "animation/",
    "front/",
    "shaders/",
    "level/cameras/",
    "level/3ds/",
    "ai/",
    "bucket/",
    "level/jpx/",
    "level/powerups/",
    "backdrop/",
    "movies/",
    "default/",
    "sound/",
    "temp/",
    "model/tga/",
    "font/",
    "platform/PC/CONTROLLER/",
    "platform/PC/CONTROLLER/",
    "platform/PC/KBM/",
    "platform/PC/PS4/",
    "platform/PC/PS5/",
    "platform/PC/SWITCH/",
    "platform/PC/SWITCHPRO/",
    "platform/PC/SWITCH/",
    "platform/PC/XBOXSERIESX/"
};

int jpb_ResourceSetBasePath(const char *base_path)
{
    size_t length;

    if (base_path == NULL) {
        basePath[0] = '\0';
        return 0;
    }
    length = strlen(base_path);
    if (length + 2 > sizeof(basePath)) {
        basePath[0] = '\0';
        return 0;
    }
    memcpy(basePath, base_path, length + 1);
    if (length != 0 && basePath[length - 1] != '/' &&
        basePath[length - 1] != '\\') {
        basePath[length++] = '/';
        basePath[length] = '\0';
    }
    return 1;
}

void jpb_ResourceSetLoadingUpdateHook(
    JPBResourceLoadingUpdateHook hook, void *user_data)
{
    loadingUpdateHook = hook;
    loadingUpdateUserData = user_data;
}

/* 0xEEDA0, 652 bytes, global, 2 named locals
 * resource_getPath
 * PDB type: const char* (const char*, Resour...
 * Source: W:\SWJediPowerBattles\work\resources.c
 */
const char *resource_getPath(
    const char *resourceName, ResourceType type)
{
    int written;

    if (resourceName == NULL || basePath[0] == '\0' ||
        (unsigned)type >= JPB_RESOURCE_TYPE_COUNT) {
        return NULL;
    }
    written = snprintf(
        path,
        sizeof(path),
        "%s%s%s%s",
        basePath,
        "res/",
        resourceDirectories[type],
        resourceName);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        path[0] = '\0';
        return NULL;
    }
    return path;
}

/* 0xEF030, 126 bytes, global, 4 named locals
 * resource_getPathWithExtension
 * PDB type: const char* (const char*, Resour...
 * Source: W:\SWJediPowerBattles\work\resources.c
 */
const char *resource_getPathWithExtension(
    const char *resourceName,
    ResourceType type,
    char *extension)
{
    char fileName[JPB_RESOURCE_PATH_CAPACITY];
    int written;

    if (resourceName == NULL || extension == NULL) {
        return NULL;
    }
    written = snprintf(
        fileName, sizeof(fileName), "%s.%s",
        resourceName, extension);
    if (written < 0 || (size_t)written >= sizeof(fileName)) {
        return NULL;
    }
    resource_updateloading();
    return resource_getPath(fileName, type);
}

/* 0xEF0B0, 55 bytes, global, 2 named locals
 * resource_pathToLower
 * PDB type: void (char*)
 * Source: W:\SWJediPowerBattles\work\resources.c
 */
void resource_pathToLower(char *resource_path)
{
    unsigned char *cursor =
        (unsigned char *)resource_path;

    if (cursor == NULL) {
        return;
    }
    while (*cursor != '\0') {
        if (isupper(*cursor)) {
            *cursor = (unsigned char)tolower(*cursor);
        }
        ++cursor;
    }
}

/* 0xEF0F0, 10 bytes, global, 0 named locals
 * resource_updateloading
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\resources.c
 */
void resource_updateloading(void)
{
    if (loadingUpdateHook != NULL) {
        loadingUpdateHook(50, loadingUpdateUserData);
    }
}

/* 0xEF100, 91 bytes, global, 4 named locals
 * snprintf
 * PDB type: int (char* const, const unsigned...
 * Source: C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt\stdio.h
 *
 * This compiler-library inline is represented by the C runtime snprintf
 * calls above and does not require a second public definition.
 */
