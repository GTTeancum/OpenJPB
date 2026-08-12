#ifndef JPB_PLATFORM_H
#define JPB_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable service boundary for platform achievement persistence. The
 * platform_* functions are exact PDB symbols; only the jpb_ registration API
 * is an inferred PC/nxdk integration seam.
 */
typedef int (*JPBPlatformCompleteAchievementHook)(
    int id, void *user_data);
typedef int (*JPBPlatformGetCompleteAchievementHook)(
    int id, void *user_data);

typedef struct JPBPlatformAchievementHooks {
    JPBPlatformCompleteAchievementHook complete;
    JPBPlatformGetCompleteAchievementHook get_complete;
} JPBPlatformAchievementHooks;

void jpb_PlatformSetAchievementHooks(
    const JPBPlatformAchievementHooks *hooks,
    void *user_data);
int platform_completeAchievement(int id);
int platform_getCompleteAchievement(int id);

#ifdef __cplusplus
}
#endif

#endif
