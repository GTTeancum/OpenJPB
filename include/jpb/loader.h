#ifndef JPB_LOADER_H
#define JPB_LOADER_H

#include "jpb/world.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct geomData geomData;
typedef struct Motion Motion;
typedef struct ufbx_scene ufbx_scene;

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
extern char *maAnimData[JPB_ANIMATION_NAME_COUNT];
extern char *maModelData[JPB_MODEL_NAME_COUNT];
extern char *sModelNames[JPB_MODEL_NAME_COUNT];
extern char *sObiNames[JPB_ACTOR_NAME_COUNT];
extern modelAnimConnect model_anim_table[JPB_ACTOR_NAME_COUNT];
/* Exact PDB globals at matched-PC RVAs 0x508580 and 0x10D7E28. */
extern int gotJPX;
extern ufbx_scene *scene;

/* Descriptive, observation-only boundary after exact enemy construction. */
typedef void (*JPBLoaderEnemyCreatedObserver)(
    wsl_ENEMY *enemy, int object_id, void *user_data);

void jpb_LoaderSetEnemyCreatedObserver(
    JPBLoaderEnemyCreatedObserver observer,
    void *user_data);
/* Portable status publication around exact local loadPowerupModels. */
size_t jpb_LoaderLoadPowerupModels(void);
geomData *getNodeByName(
    geomData *root, long buffer_size, const char *name);
int loader_CreateEnemy(wsl_ENEMY *enemy);
void initDataArrays(void);
void loader_loadJarJarOverrideModel(
    geomData **model_buffer, long *size);
char *loader_GetALevelName(int index);
char *loader_GetEnemyName(int index);
char *loader_GetLevelName(void);
char *loader_GetModelName(int index);
void loader_LevelLoad(void);

#ifdef JPB_LOADER_TESTING
typedef int (*JPBLoaderEnemyCreateTestHook)(
    wsl_ENEMY *enemy, void *user_data);
void jpb_LoaderSetEnemyCreateTestHook(
    JPBLoaderEnemyCreateTestHook hook,
    void *user_data);
void jpb_LoaderApplyLevelLoadSpecialsForTest(int level);
int jpb_LoaderFinalizeEnemyForTest(wsl_ENEMY *enemy);
void jpb_LoaderApplyEnemyModelSpecialsForTest(
    playerObject *player, Motion *motions, int model_id);
#endif

#ifdef __cplusplus
}
#endif

#endif
