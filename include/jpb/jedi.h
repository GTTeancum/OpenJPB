#ifndef JPB_JEDI_H
#define JPB_JEDI_H

#include "jpb/extracharacters.h"
#include "jpb/fmath.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_JEDI_COLOUR_COUNT = 9,
    JPB_JEDI_COLOUR_STORAGE_COUNT = 10
};

typedef struct playerObject playerObject;

extern int gJediColorSpriteLegacy[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern int gJediColorSpriteCanon[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern int gJediColorSpriteCurrent[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern CVECTOR gJediColourLegacy[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern CVECTOR gJediColourCanon[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern CVECTOR gJediColourCurrent[JPB_JEDI_COLOUR_STORAGE_COUNT];
extern int *gJediColorSprite;
extern CVECTOR *gJediColour;
extern uint64_t gJediColourArrayLength;
extern uint8_t pointLvls[16][4];
extern uint16_t award[12][4];

int CVECTOR_Equals(CVECTOR lhs, CVECTOR rhs);
void jedi_CalcBonusLevels(
    int jedi_id, int *attack_bonus, int *defend_bonus);
CVECTOR jedi_GetColour(uint64_t playerID);
uint32_t jedi_GetColour32(uint64_t playerID);
int jedi_CanToggleSaber(model_id cnum);
void jedi_CalcSkillLevels(
    int jedi_id, int *skill_percent, int *highest_level);
int jedi_CheckValidLevel(int level, int *upgrade_level);
int jedi_CheckValidPlayer(int jediID);
int jedi_CheckValidPlayerNGP(int jediID);
int jedi_CheckValidPlayerWTabs(int selectType, int jediID);
int jedi_CheckValidVersus(int jediID);
int jedi_ConvertToTextIndex(int jedi_id);
void jedi_DrawBlur(
    VECTOR *p1,
    _svector *v1,
    _svector *p2,
    _svector *v2,
    uint32_t color);
int jedi_FireWeapon(
    int32_t *cpad, playerObject *player);
int jedi_GetAwardFlags(int player_number, int points);
int jedi_HandleSabre(
    int32_t *cpad, playerObject *player);
int jedi_HasProgression(model_id cnum);
int jedi_GetColorSprite(uint64_t player_id);
int jedi_GetHighestLevel(void);
int jedi_GetLives(void);
void jedi_InitLives(void);
int jedi_InitPlayer(playerObject *player);
int jedi_IsMelee(model_id cnum);
int jedi_Main(int32_t *cpad, playerObject *player);
int jedi_SetHighestLevel(int level);
int jedi_ShowCombos(int versus_players);
void jedi_ShowSecrets(void);
void jedi_ShowStats(int jedi_id);
void jedi_ToggleSaberColor(model_id cnum);
int tusken_stab(int32_t *cpad, playerObject *player);
#ifdef __cplusplus
}
#endif

#endif
