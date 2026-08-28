#include "jpb/extracharacters.h"

#include <stdio.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                               \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int test_exact_table(void)
{
    static const ExtraCharacter expected[JPB_EXTRA_CHARACTER_COUNT] = {
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
    size_t i;

    CHECK(ExtraCharactersSize == JPB_EXTRA_CHARACTER_COUNT);
    for (i = 0; i < ExtraCharactersSize; ++i) {
        CHECK(ExtraCharacters[i].ID == expected[i].ID);
        CHECK(ExtraCharacters[i].TextIndex == expected[i].TextIndex);
        CHECK(ExtraCharacters[i].CanReflect == expected[i].CanReflect);
        CHECK(ExtraCharacters[i].CanForcePower == expected[i].CanForcePower);
        CHECK(ExtraCharacters[i].CanLedgeClimb == expected[i].CanLedgeClimb);
        CHECK(ExtraCharacters[i].Unlocked == expected[i].Unlocked);
        CHECK(GetCharacterByID(expected[i].ID) == &ExtraCharacters[i]);
        CHECK(IsExtraCharacter(expected[i].ID) == 1);
        CHECK(
            extracharacter_CanReflect(expected[i].ID) ==
            expected[i].CanReflect);
        CHECK(
            extracharacter_CanForcePower(expected[i].ID) ==
            expected[i].CanForcePower);
        CHECK(
            extracharacter_CanLedgeClimb(expected[i].ID) ==
            expected[i].CanLedgeClimb);
    }
    return 0;
}

static int test_capabilities_and_defaults(void)
{
    CHECK(extracharacter_CanReflect(battle_d_model) == 0);
    CHECK(extracharacter_CanForcePower(battle_d_model) == 1);
    CHECK(extracharacter_CanLedgeClimb(battle_d_model) == 1);

    CHECK(extracharacter_CanReflect(pilot_model) == 1);
    CHECK(extracharacter_CanLedgeClimb(destroye_model) == 0);
    CHECK(extracharacter_CanLedgeClimb(loader_model) == 0);

    CHECK(GetCharacterByID(obi_wan_model) == NULL);
    CHECK(IsExtraCharacter(obi_wan_model) == 0);
    CHECK(extracharacter_CanReflect(obi_wan_model) == 0);
    CHECK(extracharacter_CanForcePower(obi_wan_model) == 0);
    CHECK(extracharacter_CanLedgeClimb(obi_wan_model) == 1);
    return 0;
}

static int test_unlock_is_mutable(void)
{
    int original = ExtraCharacters[0].Unlocked;

    ExtraCharacters[0].Unlocked = 0;
    CHECK(GetCharacterByID(battle_d_model)->Unlocked == 0);
    ExtraCharacters[0].Unlocked = original;
    CHECK(ExtraCharacters[0].Unlocked == 1);
    return 0;
}

int main(void)
{
    if (test_exact_table() != 0) {
        return 1;
    }
    if (test_capabilities_and_defaults() != 0) {
        return 1;
    }
    if (test_unlock_is_mutable() != 0) {
        return 1;
    }

    puts("extracharacter tests passed");
    return 0;
}
