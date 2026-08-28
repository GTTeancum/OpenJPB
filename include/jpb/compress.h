#ifndef JPB_COMPRESS_H
#define JPB_COMPRESS_H

#include "jpb/fmath.h"

#ifdef __cplusplus
extern "C" {
#endif

void comp_AddFrames(_svector *last, _svector *pNew, int D_PartCount);
unsigned char *comp_GetDeltaFrame(
    unsigned char *addr,
    _svector *Dest,
    short first,
    int D_PartCount);
int comp_GetFrameTrans(unsigned char *addr, _svector *Dest);
void comp_GetSVector(_svector *Vect, short pdsz);

#ifdef __cplusplus
}
#endif

#endif
