#ifndef JPB_PLATFORM_H
#define JPB_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Exact PDB platform entry points plus deterministic test interception. */
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
int jpb_PlatformInitializeSteamServices(void);
void jpb_PlatformShutdownSteamServices(void);
int platform_completeAchievement(int id);
int platform_getCompleteAchievement(int id);
unsigned char platform_getSystemLanguage(void);
int platform_openURL(const char *url);
void platform_update(void);

#ifdef __cplusplus
}
#endif

#endif
