#ifndef JPB_ACHIEVEMENT_H
#define JPB_ACHIEVEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

enum { JPB_ACHIEVEMENT_COUNT = 44 };

extern int achievements[JPB_ACHIEVEMENT_COUNT];

void achievement_checkForPlatinum(int id);
void achievement_complete(int id);
void achievement_destroy(void);
int achievement_getcomplete(int id);
int achievement_getcount(int id);
void achievement_update(int id, int count);

#ifdef __cplusplus
}
#endif

#endif
