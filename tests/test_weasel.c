#include "jpb/weasel.h"

#include "jpb/combo.h"
#include "jpb/effects.h"
#include "jpb/whook.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n",                \
                __FILE__, __LINE__, #condition);                             \
            return 0;                                                        \
        }                                                                    \
    } while (0)

enum {
    VK_LEFT_TEST = 0x25,
    VK_UP_TEST = 0x26,
    VK_RIGHT_TEST = 0x27,
    VK_DOWN_TEST = 0x28
};

static Motion motions[80];
static Combo combos[24];
static playerObject player;

static void press_key(int virtual_key)
{
    jpb_WHookClearKeyState();
    jpb_WHookHandleKeyEvent(virtual_key, 0, 1);
}

static void press_two_keys(int first, int second)
{
    jpb_WHookClearKeyState();
    jpb_WHookHandleKeyEvent(first, 0, 1);
    jpb_WHookHandleKeyEvent(second, 0, 1);
}

static void reset_fixture(void)
{
    memset(motions, 0, sizeof(motions));
    memset(combos, 0, sizeof(combos));
    memset(&player, 0, sizeof(player));
    player.paMotions = motions;
    player.maxMotions = 70;
    player.paCombos = combos;
    paMotions = motions;
    fFrame = 10;
    lFrame = 20;
    jpb_WHookClearKeyState();
}

static int test_basic_charge_step_and_bounds(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    motions[0].Charge = 0x200;
    press_key(VK_UP_TEST);
    weasel_EditBasic(cpad, &player, 0);
    CHECK(motions[0].Charge == 0x200);
    press_key(VK_DOWN_TEST);
    weasel_EditBasic(cpad, &player, 0);
    CHECK(motions[0].Charge == 0x1fc);
    return 1;
}

static int test_combat_minus_one_byte_encoding(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    motions[0].combo = 0;
    press_key(VK_DOWN_TEST);
    weasel_EditCombat(cpad, &player, 0);
    CHECK(motions[0].combo == 0xff);
    press_key(VK_UP_TEST);
    weasel_EditCombat(cpad, &player, 0);
    CHECK(motions[0].combo == 0x41);
    return 1;
}

static int test_effect_id_uses_inclusive_global_bound(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    gMaxEffect = 12;
    motions[0].fx2 = 12;
    press_key(VK_UP_TEST);
    weasel_EditEffects(cpad, &player, 0);
    CHECK(motions[0].fx2 == 12);
    press_key(VK_DOWN_TEST);
    weasel_EditEffects(cpad, &player, 0);
    CHECK(motions[0].fx2 == 11);
    return 1;
}

static int test_flags_editor_excludes_loop_selector(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    press_key(VK_UP_TEST);
    weasel_EditFlags(cpad, &player, 0);
    CHECK(motions[0].motionFlags == 0x08000000U);
    press_key(VK_DOWN_TEST);
    weasel_EditFlags(cpad, &player, 0);
    CHECK(motions[0].motionFlags == 0);
    return 1;
}

static int test_mechanics_display_frame_bound(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    fFrame = 30;
    lFrame = 35;
    motions[0].disp = 5;
    press_key(VK_UP_TEST);
    weasel_EditMechanics(cpad, &player, 0);
    CHECK(motions[0].disp == 5);
    press_key(VK_DOWN_TEST);
    weasel_EditMechanics(cpad, &player, 0);
    CHECK(motions[0].disp == 4);
    return 1;
}

static int test_frame_editor_preserves_byte_wraparound(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    motions[0].cutin = 0xff;
    press_two_keys('F', VK_RIGHT_TEST);
    weasel_EditFrames(cpad, &player, 0);
    CHECK(motions[0].cutin == 0);
    CHECK(motions[0].cutout == 0);
    return 1;
}

static int test_combo_underflow_clamps_to_max_motion(void)
{
    uint32_t cpad[2] = {0};

    reset_fixture();
    player.maxMotions = 10;
    press_key(VK_UP_TEST);
    CHECK(weasel_EditCombo(cpad, &player) == 0);
    CHECK(combos[0].comboFlags == 0x04000000U);

    press_key(VK_RIGHT_TEST);
    CHECK(weasel_EditCombo(cpad, &player) == 0);
    combos[0].Index = 0;
    press_key(VK_DOWN_TEST);
    CHECK(weasel_EditCombo(cpad, &player) == 0);
    CHECK(combos[0].Index == 10);
    return 1;
}

int main(void)
{
    CHECK(test_basic_charge_step_and_bounds());
    CHECK(test_combat_minus_one_byte_encoding());
    CHECK(test_effect_id_uses_inclusive_global_bound());
    CHECK(test_flags_editor_excludes_loop_selector());
    CHECK(test_mechanics_display_frame_bound());
    CHECK(test_frame_editor_preserves_byte_wraparound());
    CHECK(test_combo_underflow_clamps_to_max_motion());
    weasel_DumpFlags(0xdc000000U);
    weasel_LevelBars(1, 2, 3);
    puts("weasel tests passed");
    return 0;
}
