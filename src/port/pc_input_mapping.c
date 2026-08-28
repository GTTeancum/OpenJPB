#include "pc_input_mapping.h"

#include "jpb/input.h"

uint32_t jpb_PCMapGameplayKeyboard(
    const JPBPCGameplayKeyboardState *keyboard,
    float *axis_x,
    float *axis_y)
{
    return jpb_PCMapKeyboard(
        keyboard, 0, 0, axis_x, axis_y);
}

uint32_t jpb_PCMapKeyboard(
    const JPBPCGameplayKeyboardState *keyboard,
    int in_menu_state,
    int in_title_state,
    float *axis_x,
    float *axis_y)
{
    return jpb_InputMapKeyboardState(
        keyboard, in_menu_state, in_title_state, axis_x, axis_y);
}

int jpb_PCControllerUserForPlayer(
    unsigned player,
    int player_count,
    int player_two_user,
    uint32_t connected_mask,
    unsigned *user_index)
{
    unsigned user;

    if (player > 1 || user_index == NULL ||
        connected_mask == 0 ||
        (player == 1 && player_count != 2)) {
        return 0;
    }
    if (player == 1) {
        if (player_two_user < 0 ||
            player_two_user >= 4 ||
            (connected_mask &
             (UINT32_C(1) << player_two_user)) == 0) {
            return 0;
        }
        *user_index = (unsigned)player_two_user;
        return 1;
    }

    for (user = 0; user < 4; ++user) {
        if ((connected_mask & (UINT32_C(1) << user)) != 0 &&
            (player_count != 2 ||
             (int)user != player_two_user)) {
            *user_index = user;
            return 1;
        }
    }
    return 0;
}

int jpb_PCChoosePlayerTwoUser(
    int player_one_uses_keyboard,
    uint32_t connected_mask,
    unsigned *user_index)
{
    unsigned user;
    int skip_first = !player_one_uses_keyboard;

    if (user_index == NULL) {
        return 0;
    }
    for (user = 0; user < 4; ++user) {
        if ((connected_mask & (UINT32_C(1) << user)) == 0) {
            continue;
        }
        if (skip_first) {
            skip_first = 0;
            continue;
        }
        *user_index = user;
        return 1;
    }
    return 0;
}

int jpb_PCChoosePlayerTwoAddedUser(
    int player_one_uses_keyboard,
    uint32_t connected_mask,
    uint32_t added_mask,
    unsigned *user_index)
{
    uint32_t eligible_mask = connected_mask & added_mask;
    unsigned user;

    if (user_index == NULL) {
        return 0;
    }
    if (!player_one_uses_keyboard) {
        for (user = 0; user < 4; ++user) {
            uint32_t user_bit = UINT32_C(1) << user;

            if ((connected_mask & user_bit) != 0) {
                eligible_mask &= ~user_bit;
                break;
            }
        }
    }
    for (user = 0; user < 4; ++user) {
        if ((eligible_mask & (UINT32_C(1) << user)) != 0) {
            *user_index = user;
            return 1;
        }
    }
    return 0;
}

uint32_t jpb_PCSelectPlayerOneInput(
    uint32_t keyboard_bits,
    uint32_t controller_bits)
{
    return keyboard_bits != 0 ? keyboard_bits : controller_bits;
}

uint32_t jpb_PCSelectPlayerOneOwnedInput(
    uint32_t keyboard_bits,
    uint32_t controller_bits,
    int player_one_uses_keyboard)
{
    if (keyboard_bits != 0) {
        return keyboard_bits;
    }
    return player_one_uses_keyboard ? 0 : controller_bits;
}
