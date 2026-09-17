/* Initial Xbox milestone uses stock assets only. No mod/save success is faked. */
#include "jpb/mods.h"
#include <stdio.h>
int jpb_ModsLoad(const char *p,char *e,size_t n){if(e&&n)snprintf(e,n,"Xbox mod loading not implemented");return 0;}
void jpb_ModsReset(void){}
void jpb_ModResetProgress(void){}
int jpb_ModSaveCommit(const char*p,const void*d,size_t n,const int m[2]){return 0;}
int jpb_ModSaveRestore(const char*p,const void*d,size_t n,int m[2]){return -1;}
const char *jpb_ModSaveError(void){return "Xbox mod saves not implemented";}
int jpb_ModRecordLevel(int m,int l,uint32_t s){return 0;}
int jpb_ModLevelPlayed(int m,int l){return 0;}
uint32_t jpb_ModLevelScore(int m,int l){return 0;}
uint8_t *jpb_ModComboMask(int m,const uint8_t a[6]){return NULL;}
struct Upgrades *jpb_ModUpgrades(int m){return NULL;}
int jpb_ModsSetAssetRoot(const char*p){return 1;}
int jpb_ModsResolvePath(const char*p,char*o,size_t n){return 0;}
int jpb_ModsBasePath(const char*p,char*o,size_t n){return 0;}
size_t jpb_ModsCount(void){return 0;}
const JPBModCharacter *jpb_ModCharacterById(int m){return NULL;}
const JPBModCharacter *jpb_ModCharacterAt(size_t m){return NULL;}
int jpb_ModDonor(int m){return m;}
int jpb_ModLastSelectable(int m){return m;}
int jpb_ModToggleColor(int m){return 0;}
int jpb_ModSelectColor(int m,int a){return 0;}
const char *jpb_ModSaberIconPath(int m){return NULL;}
void jpb_ModSetPlayer(int p,int m){}
const JPBModCharacter *jpb_ModPlayer(int p){return NULL;}
int jpb_ModPlayerModel(int p,int fallback){return fallback;}
int jpb_ModsResolve(const char*p,char*o,size_t n){return 0;}
