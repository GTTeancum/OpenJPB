#ifndef JPB_LOADER_H
#define JPB_LOADER_H

#include "jpb/world.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_ANIMATION_NAME_COUNT = 46,
    JPB_MODEL_NAME_COUNT = 115,
    JPB_ACTOR_NAME_COUNT = 115
};

typedef struct modelAnimConnect {
    uint8_t modelID;
    uint8_t poolID;
    int8_t poolOffset;
    int8_t pad;
} modelAnimConnect;

extern char *sAnimNames[JPB_ANIMATION_NAME_COUNT];
extern char *sModelNames[JPB_MODEL_NAME_COUNT];
extern char *sObiNames[JPB_ACTOR_NAME_COUNT];
extern modelAnimConnect model_anim_table[JPB_ACTOR_NAME_COUNT];

/*
 * Portable boundary for the model/animation construction performed by the
 * original loader_CreateEnemy. The callback and setter names are inferred;
 * loader_CreateEnemy itself is an exact PDB procedure name.
 */
typedef int (*JPBLoaderEnemyCreateProvider)(
    wsl_ENEMY *enemy, void *user_data);

void jpb_LoaderSetEnemyCreateProvider(
    JPBLoaderEnemyCreateProvider provider,
    void *user_data);
int loader_CreateEnemy(wsl_ENEMY *enemy);
char *loader_GetALevelName(int index);
char *loader_GetEnemyName(int index);
char *loader_GetLevelName(void);
char *loader_GetModelName(int index);

#ifdef __cplusplus
}
#endif

#endif
