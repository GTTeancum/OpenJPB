#ifndef JPB_RESOURCES_H
#define JPB_RESOURCES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { JPB_RESOURCE_PATH_CAPACITY = 256 };

/*
 * Exact ResourceType numeric mapping recovered from resource_getPath. The
 * descriptive enumerator spellings are reconstruction names because the PDB
 * retained the enum type but not its individual enumerator names.
 */
typedef enum ResourceType {
    JPB_RESOURCE_SOUND_SFX_FINAL = 0,
    JPB_RESOURCE_SOUND_STREAMS = 1,
    JPB_RESOURCE_EFFECT_TEXTURE = 2,
    JPB_RESOURCE_COMBO = 3,
    JPB_RESOURCE_EFFECT = 4,
    JPB_RESOURCE_LEVEL_W3D = 5,
    JPB_RESOURCE_MODEL = 6,
    JPB_RESOURCE_ANIMATION = 7,
    JPB_RESOURCE_FRONT = 8,
    JPB_RESOURCE_SHADER = 9,
    JPB_RESOURCE_LEVEL_CAMERA = 10,
    JPB_RESOURCE_LEVEL_3DS = 11,
    JPB_RESOURCE_AI = 12,
    JPB_RESOURCE_BUCKET = 13,
    JPB_RESOURCE_LEVEL_JPX = 14,
    JPB_RESOURCE_LEVEL_POWERUP = 15,
    JPB_RESOURCE_BACKDROP = 16,
    JPB_RESOURCE_MOVIE = 17,
    JPB_RESOURCE_DEFAULT = 18,
    JPB_RESOURCE_SOUND = 19,
    JPB_RESOURCE_TEMP = 20,
    JPB_RESOURCE_MODEL_TEXTURE = 21,
    JPB_RESOURCE_FONT = 22,
    JPB_RESOURCE_CONTROLLER_PRIMARY = 23,
    JPB_RESOURCE_CONTROLLER_SECONDARY = 24,
    JPB_RESOURCE_KEYBOARD_MOUSE = 25,
    JPB_RESOURCE_PS4 = 26,
    JPB_RESOURCE_PS5 = 27,
    JPB_RESOURCE_SWITCH = 28,
    JPB_RESOURCE_SWITCH_PRO = 29,
    JPB_RESOURCE_SWITCH_SECONDARY = 30,
    JPB_RESOURCE_XBOX_SERIES_X = 31,
    JPB_RESOURCE_TYPE_COUNT = 32
} ResourceType;

typedef void (*JPBResourceLoadingUpdateHook)(
    unsigned milliseconds, void *user_data);

const char *resource_getPath(
    const char *resourceName, ResourceType type);
const char *resource_getPathWithExtension(
    const char *resourceName,
    ResourceType type,
    char *extension);
void resource_pathToLower(char *path);
void resource_updateloading(void);

/*
 * Dependency-light platform seams replacing SDL_GetBasePath and Sleep at
 * the edge of the original resource owner.
 */
int jpb_ResourceSetBasePath(const char *base_path);
void jpb_ResourceSetLoadingUpdateHook(
    JPBResourceLoadingUpdateHook hook, void *user_data);

#ifdef __cplusplus
}
#endif

#endif
