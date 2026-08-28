#ifndef JPB_LINKSTUBS_H
#define JPB_LINKSTUBS_H

#include "jpb/fmath.h"
#include "jpb/prim.h"

typedef struct POLY_G4 POLY_G4;

#ifdef __cplusplus
extern "C" {
#endif

/* Exact retail compatibility stub at matched-PC RVA 0xBB8F0. */
void CopyMemLong(void *destination, void *source, unsigned long length);
unsigned *ClearOTagR(unsigned *ordering_table, int length);
int OpenTIM(unsigned *address);
int DrawSync(int mode);
void DrawPrim(void *primitive);
int LoadImage(SRECT *rectangle, unsigned *source);
unsigned short LoadClut(unsigned *source, int x, int y);
DRAWENV *PutDrawEnv(DRAWENV *environment);
DRAWENV *SetDefDrawEnv(
    DRAWENV *environment, int x, int y, int width, int height);
int StoreImage(SRECT *rectangle, unsigned *destination);
void SetCameraMatrix(void);
void SetPolyG4(POLY_G4 *primitive);
void SetRotMatrix(MATRIX *matrix);
void SetSemiTrans(void *primitive, int enabled);
void SetShadeTex(void *primitive, int enabled);
void SetTransMatrix(MATRIX *matrix);
char *getScratchAddr(int offset);
int platform_completeLevel(char level);
int platform_enterLevel(char level);
int platform_isSuspended(void);
void closeFileLog(void);
/* Exact retail compatibility stub at matched-PC RVA 0xBBA30. */
void _HandleBackDrop(void);

#ifdef __cplusplus
}
#endif

#endif
