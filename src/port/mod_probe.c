#include "jpb/mods.h"
#include <stdio.h>
int main(int argc,char **argv){
    char error[2048],path[JPB_MOD_PATH];
    if(argc<2||argc>3)return 2;
    if(!jpb_ModsLoad(argv[1],error,sizeof(error))){fprintf(stderr,"%s\n",error);return 1;}
    printf("characters=%zu\n",jpb_ModsCount());
    for(size_t i=0;i<jpb_ModsCount();++i){const JPBModCharacter *c=jpb_ModCharacterAt(i);
        printf("id=%s model_id=%d donor=%d force=%d name=%s\nbmd=%s\ncad=%s\ncmb=%s\nportrait=%s\n",c->id,c->modelId,c->animationDonor,c->forceDonor,c->name,c->bmd,c->cad,c->cmb,c->portrait);}
    if(argc==3){if(!jpb_ModsResolve(argv[2],path,sizeof(path)))return 3;printf("resolved=%s\n",path);}
    return 0;
}
