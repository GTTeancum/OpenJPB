#include "jpb/extracharacters.h"

/*
 * Exact 14-entry table at matched-PC RVA 0x4B8B00. Field names and layout
 * come from PDB type 0x6C4D; values were read from the paired executable.
 */
ExtraCharacter ExtraCharacters[JPB_EXTRA_CHARACTER_COUNT] = {
    {battle_d_model, 342, 0, 1, 1, 1},
    {pilot_model, 341, 1, 1, 1, 0},
    {rifle_model, 343, 0, 1, 1, 0},
    {flame_model, 344, 1, 1, 1, 0},
    {destroye_model, 346, 0, 1, 0, 0},
    {loader_model, 347, 1, 1, 0, 0},
    {tusken_s_model, 348, 1, 1, 1, 0},
    {tusken_r_model, 349, 0, 1, 1, 0},
    {thug_1_model, 351, 0, 1, 1, 0},
    {thug_2_model, 352, 1, 1, 1, 0},
    {thug_3_model, 353, 0, 1, 1, 0},
    {thug_4_model, 354, 1, 1, 1, 0},
    {jar_jar_playable_model, 357, 1, 1, 1, 0},
    {gungan_1_model, 356, 1, 1, 1, 0}
};

/* Exact const global at matched-PC RVA 0x335810. */
const size_t ExtraCharactersSize = JPB_EXTRA_CHARACTER_COUNT;

/* 0x4BB60, 46 bytes, global, 2 named locals
 * GetCharacterByID
 * PDB type: ExtraCharacter* (model_id)
 * Source: W:\SWJediPowerBattles\work\extracharacters.c
 */
ExtraCharacter *GetCharacterByID(model_id ID)
{
    size_t i;

    for (i = 0; i < ExtraCharactersSize; ++i) {
        if (ExtraCharacters[i].ID == ID) {
            return &ExtraCharacters[i];
        }
    }
    return NULL;
}

/* 0x4BB90, 67 bytes, global, 2 named locals
 * IsExtraCharacter
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\work\extracharacters.c
 */
int IsExtraCharacter(model_id ID)
{
    size_t i;

    for (i = 0; i < ExtraCharactersSize; ++i) {
        if (ExtraCharacters[i].ID == ID) {
            return 1;
        }
    }
    return 0;
}

/* 0x4BBE0, 54 bytes, global, 3 named locals
 * extracharacter_CanForcePower
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\work\extracharacters.c
 */
int extracharacter_CanForcePower(model_id ID)
{
    const ExtraCharacter *Character = GetCharacterByID(ID);

    return Character != NULL ? Character->CanForcePower : 0;
}

/* 0x4BC20, 57 bytes, global, 3 named locals
 * extracharacter_CanLedgeClimb
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\work\extracharacters.c
 */
int extracharacter_CanLedgeClimb(model_id ID)
{
    const ExtraCharacter *Character = GetCharacterByID(ID);

    return Character != NULL ? Character->CanLedgeClimb : 1;
}

/* 0x4BC60, 54 bytes, global, 3 named locals
 * extracharacter_CanReflect
 * PDB type: int (model_id)
 * Source: W:\SWJediPowerBattles\work\extracharacters.c
 */
int extracharacter_CanReflect(model_id ID)
{
    const ExtraCharacter *Character = GetCharacterByID(ID);

    return Character != NULL ? Character->CanReflect : 0;
}
