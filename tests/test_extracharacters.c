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
    static const model_id expected_ids[JPB_EXTRA_CHARACTER_COUNT] = {
        battle_d_model,
        pilot_model,
        rifle_model,
        flame_model,
        destroye_model,
        loader_model,
        tusken_s_model,
        tusken_r_model,
        thug_1_model,
        thug_2_model,
        thug_3_model,
        thug_4_model,
        jar_jar_playable_model,
        gungan_1_model
    };
    static const int expected_text[JPB_EXTRA_CHARACTER_COUNT] = {
        342, 341, 343, 344, 346, 347, 348,
        349, 351, 352, 353, 354, 357, 356
    };
    size_t i;

    CHECK(ExtraCharactersSize == JPB_EXTRA_CHARACTER_COUNT);
    for (i = 0; i < ExtraCharactersSize; ++i) {
        CHECK(ExtraCharacters[i].ID == expected_ids[i]);
        CHECK(ExtraCharacters[i].TextIndex == expected_text[i]);
        CHECK(GetCharacterByID(expected_ids[i]) == &ExtraCharacters[i]);
        CHECK(IsExtraCharacter(expected_ids[i]) == 1);
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
