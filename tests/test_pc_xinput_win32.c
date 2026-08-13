#include "pc_xinput_win32.h"
#include "pc_input_mapping.h"

#include "jpb/input.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr, "CHECK failed at %s:%d: %s\n",                     \
                __FILE__, __LINE__, #condition);                             \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct FakeXInputRuntime {
    uint32_t connectedMask;
    JPBPCXInputStatePacket states[4];
    unsigned getCalls[4];
    unsigned setCalls[4];
    JPBPCXInputVibration lastVibration[4];
} FakeXInputRuntime;

static FakeXInputRuntime fake_xinput;

static DWORD WINAPI fake_xinput_get_state(
    DWORD user_index, JPBPCXInputStatePacket *state)
{
    if (user_index >= 4 ||
        (fake_xinput.connectedMask &
         (UINT32_C(1) << user_index)) == 0) {
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    ++fake_xinput.getCalls[user_index];
    *state = fake_xinput.states[user_index];
    return ERROR_SUCCESS;
}

static DWORD WINAPI fake_xinput_set_state(
    DWORD user_index, JPBPCXInputVibration *vibration)
{
    if (user_index >= 4 ||
        (fake_xinput.connectedMask &
         (UINT32_C(1) << user_index)) == 0) {
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    ++fake_xinput.setCalls[user_index];
    fake_xinput.lastVibration[user_index] = *vibration;
    return ERROR_SUCCESS;
}

static int test_exact_button_layout(void)
{
    JPBPCXInputGamepad gamepad;
    uint32_t bits;

    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.buttons =
        JPB_PC_XINPUT_A | JPB_PC_XINPUT_B |
        JPB_PC_XINPUT_X | JPB_PC_XINPUT_Y |
        JPB_PC_XINPUT_LEFT_SHOULDER |
        JPB_PC_XINPUT_RIGHT_SHOULDER |
        JPB_PC_XINPUT_BACK | JPB_PC_XINPUT_START;
    gamepad.leftTrigger = 1;
    gamepad.rightTrigger = 1;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 0, 0, NULL, NULL);
    CHECK(bits ==
          (JPB_PAD_COMBO_SOUTH | JPB_PAD_JUMP |
           JPB_PAD_COMBO_WEST | JPB_PAD_COMBO_NORTH |
           JPB_PAD_BLOCK | JPB_PAD_LOCK_ON |
           JPB_PAD_ZOOM_IN | JPB_PAD_START |
           JPB_PAD_LEFT_TRIGGER | JPB_PAD_RIGHT_TRIGGER));

    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.buttons = JPB_PC_XINPUT_A;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, NULL, NULL);
    CHECK(bits == JPB_PAD_JUMP);
    gamepad.buttons = JPB_PC_XINPUT_B;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, NULL, NULL);
    CHECK(bits == JPB_PAD_COMBO_NORTH);
    gamepad.buttons = JPB_PC_XINPUT_X;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, NULL, NULL);
    CHECK(bits == JPB_PAD_COMBO_WEST);
    gamepad.buttons = JPB_PC_XINPUT_Y;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, NULL, NULL);
    CHECK(bits == JPB_PAD_COMBO_SOUTH);

    /* Menu input ignores the gameplay scheme and restores A/B navigation. */
    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.buttons = JPB_PC_XINPUT_A;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 1, NULL, NULL);
    CHECK(bits == JPB_PAD_COMBO_SOUTH);
    gamepad.buttons = JPB_PC_XINPUT_B;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 1, NULL, NULL);
    CHECK(bits == JPB_PAD_JUMP);
    return 0;
}

static int test_directions_and_thresholds(void)
{
    JPBPCXInputGamepad gamepad;
    float axis_x;
    float axis_y;
    uint32_t bits;

    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.thumbLX = 655;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK(bits == 0);
    CHECK(axis_x == 0.0f && axis_y == 0.0f);

    gamepad.thumbLX = 656;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_LEFT) != 0);
    CHECK((bits & JPB_PAD_ANALOG_MOVEMENT) == 0);
    CHECK(axis_x > 0.020f && axis_x < 0.021f);
    CHECK(axis_y == 0.0f);

    gamepad.thumbLX = 2621;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_LEFT) != 0);
    CHECK((bits & JPB_PAD_ANALOG_MOVEMENT) == 0);

    gamepad.thumbLX = 2622;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_LEFT) != 0);
    CHECK((bits & JPB_PAD_ANALOG_MOVEMENT) != 0);
    CHECK(axis_x > 0.080f && axis_x < 0.081f);
    CHECK(axis_y == 0.0f);

    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.buttons = JPB_PC_XINPUT_DPAD_UP |
                      JPB_PC_XINPUT_DPAD_RIGHT;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_ANALOG_MOVEMENT) == 0);
    CHECK((bits & JPB_PAD_LEFT) != 0);
    CHECK((bits & JPB_PAD_UP) != 0);
    CHECK(axis_x == 1.0f);
    CHECK(axis_y == -1.0f);

    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.buttons = JPB_PC_XINPUT_DPAD_DOWN |
                      JPB_PC_XINPUT_DPAD_LEFT;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_RIGHT) != 0);
    CHECK((bits & JPB_PAD_DOWN) != 0);
    CHECK(axis_x == -1.0f);
    CHECK(axis_y == 1.0f);

    /* D-pad contributions are added to the live analog values. */
    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.thumbLX = 1000;
    gamepad.buttons = JPB_PC_XINPUT_DPAD_RIGHT;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 2, 8, 1, 0, &axis_x, &axis_y);
    CHECK((bits & JPB_PAD_LEFT) != 0);
    CHECK((bits & JPB_PAD_ANALOG_MOVEMENT) == 0);
    CHECK(axis_x > 1.030f && axis_x < 1.031f);

    /* Percentages above 100 are clamped exactly as ReadJoystickInput. */
    memset(&gamepad, 0, sizeof(gamepad));
    gamepad.thumbLX = 32767;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 255, 255, 1, 0, &axis_x, &axis_y);
    CHECK(bits == 0);
    CHECK(axis_x == 0.0f && axis_y == 0.0f);

    /* RAND_MAX normalization preserves SDL's asymmetric negative endpoint. */
    gamepad.thumbLX = INT16_MIN;
    bits = jpb_PCXInputMapGamepad(
        &gamepad, 255, 255, 1, 0, &axis_x, &axis_y);
    CHECK(bits == (JPB_PAD_RIGHT | JPB_PAD_ANALOG_MOVEMENT));
    CHECK(axis_x < -1.0f && axis_x > -1.001f);
    CHECK(axis_y == 0.0f);
    return 0;
}

static int test_exact_gameplay_keyboard_layout(void)
{
    JPBPCGameplayKeyboardState keyboard;
    float axis_x;
    float axis_y;
    uint32_t bits;

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.moveUp = 1;
    keyboard.moveLeft = 1;
    keyboard.walkModifier = 1;
    bits = jpb_PCMapGameplayKeyboard(&keyboard, &axis_x, &axis_y);
    CHECK(bits ==
          (JPB_PAD_UP | JPB_PAD_RIGHT |
           JPB_PAD_ANALOG_MOVEMENT));
    CHECK(axis_x == -1.0f);
    CHECK(axis_y == -1.0f);

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.moveDown = 1;
    keyboard.moveRight = 1;
    keyboard.zoomIn = 1;
    keyboard.comboSouth = 1;
    keyboard.comboWest = 1;
    keyboard.comboNorth = 1;
    keyboard.lockOn = 1;
    keyboard.start = 1;
    bits = jpb_PCMapGameplayKeyboard(&keyboard, &axis_x, &axis_y);
    CHECK(bits ==
          (JPB_PAD_DOWN | JPB_PAD_LEFT | JPB_PAD_ZOOM_IN |
           JPB_PAD_COMBO_SOUTH | JPB_PAD_COMBO_WEST |
           JPB_PAD_COMBO_NORTH | JPB_PAD_LOCK_ON |
           JPB_PAD_START));
    CHECK(axis_x == 1.0f);
    CHECK(axis_y == 1.0f);

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.jumpBlockChord = 1;
    CHECK(jpb_PCMapGameplayKeyboard(&keyboard, NULL, NULL) == 0x25u);
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.northBlockChord = 1;
    CHECK(jpb_PCMapGameplayKeyboard(&keyboard, NULL, NULL) == 0x15u);
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.southBlockChord = 1;
    CHECK(jpb_PCMapGameplayKeyboard(&keyboard, NULL, NULL) == 0x45u);
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.westBlockChord = 1;
    CHECK(jpb_PCMapGameplayKeyboard(&keyboard, NULL, NULL) == 0x85u);
    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.block = 1;
    keyboard.jump = 1;
    CHECK(jpb_PCMapGameplayKeyboard(&keyboard, NULL, NULL) == 0x125u);
    return 0;
}

static int test_exact_menu_keyboard_layout(void)
{
    JPBPCGameplayKeyboardState keyboard;

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.block = 1;
    CHECK(jpb_PCMapKeyboard(
        &keyboard, 1, 0, NULL, NULL) == JPB_PAD_BLOCK);

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.start = 1;
    CHECK(jpb_PCMapKeyboard(
        &keyboard, 1, 0, NULL, NULL) == JPB_PAD_JUMP);

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.jump = 1;
    CHECK(jpb_PCMapKeyboard(
        &keyboard, 1, 0, NULL, NULL) == JPB_PAD_START);
    CHECK(jpb_PCMapKeyboard(
        &keyboard, 1, 1, NULL, NULL) == JPB_PAD_COMBO_SOUTH);

    keyboard.comboWest = 1;
    CHECK(jpb_PCMapKeyboard(
        &keyboard, 1, 1, NULL, NULL) ==
        (JPB_PAD_COMBO_SOUTH | JPB_PAD_COMBO_WEST));
    return 0;
}

static int test_controller_ownership(void)
{
    unsigned user_index = 99;

    CHECK(jpb_PCControllerUserForPlayer(0, 1, -1, 1u, &user_index));
    CHECK(user_index == 0);
    CHECK(!jpb_PCControllerUserForPlayer(1, 1, -1, 1u, &user_index));

    /* Controller-owned P1 reserves user zero and P2 joins on user one. */
    CHECK(jpb_PCChoosePlayerTwoUser(0, 3u, &user_index));
    CHECK(user_index == 1);
    CHECK(jpb_PCControllerUserForPlayer(0, 2, 1, 3u, &user_index));
    CHECK(user_index == 0);
    CHECK(jpb_PCControllerUserForPlayer(1, 2, 1, 3u, &user_index));
    CHECK(user_index == 1);

    /* Keyboard P1 plus a single controller is a valid two-player layout. */
    CHECK(jpb_PCChoosePlayerTwoUser(1, 1u, &user_index));
    CHECK(user_index == 0);
    CHECK(!jpb_PCControllerUserForPlayer(0, 2, 0, 1u, &user_index));
    CHECK(jpb_PCControllerUserForPlayer(1, 2, 0, 1u, &user_index));
    CHECK(user_index == 0);

    /* With two pads, P1 can fall back to the one not reserved by P2. */
    CHECK(jpb_PCControllerUserForPlayer(0, 2, 0, 3u, &user_index));
    CHECK(user_index == 1);
    CHECK(!jpb_PCControllerUserForPlayer(2, 2, 0, 3u, &user_index));
    CHECK(!jpb_PCControllerUserForPlayer(0, 1, -1, 1u, NULL));
    CHECK(!jpb_PCChoosePlayerTwoUser(0, 1u, &user_index));
    CHECK(!jpb_PCChoosePlayerTwoUser(1, 1u, NULL));

    /* A disconnected P2 is restored by a newly attached eligible user. */
    CHECK(jpb_PCChoosePlayerTwoAddedUser(
        1, UINT32_C(0x04), UINT32_C(0x04), &user_index));
    CHECK(user_index == 2);
    CHECK(jpb_PCChoosePlayerTwoAddedUser(
        0, UINT32_C(0x0c), UINT32_C(0x08), &user_index));
    CHECK(user_index == 3);
    CHECK(!jpb_PCChoosePlayerTwoAddedUser(
        0, UINT32_C(0x0c), UINT32_C(0x04), &user_index));
    CHECK(!jpb_PCChoosePlayerTwoAddedUser(
        1, UINT32_C(0x0c), 0, &user_index));
    CHECK(!jpb_PCChoosePlayerTwoAddedUser(
        1, UINT32_C(0x0c), UINT32_C(0x08), NULL));

    CHECK(jpb_PCSelectPlayerOneInput(0x1000u, 0x4000u) == 0x1000u);
    CHECK(jpb_PCSelectPlayerOneInput(0, 0x4000u) == 0x4000u);
    CHECK(jpb_PCSelectPlayerOneOwnedInput(
        0x2000u, 0x4000u, 1) == 0x2000u);
    CHECK(jpb_PCSelectPlayerOneOwnedInput(
        0, 0x4000u, 1) == 0);
    CHECK(jpb_PCSelectPlayerOneOwnedInput(
        0, 0x4000u, 0) == 0x4000u);
    return 0;
}

static int test_sparse_physical_user_routing(void)
{
    JPBPCXInput xinput;
    uint32_t bits;
    float axis_x;
    float axis_y;
    unsigned player_one_user = 99;
    unsigned player_two_user = 99;

    memset(&fake_xinput, 0, sizeof(fake_xinput));
    memset(&xinput, 0, sizeof(xinput));
    fake_xinput.connectedMask = UINT32_C(0x0c);
    fake_xinput.states[2].gamepad.buttons = JPB_PC_XINPUT_A;
    fake_xinput.states[3].gamepad.buttons = JPB_PC_XINPUT_B;
    xinput.getState = fake_xinput_get_state;
    xinput.setState = fake_xinput_set_state;

    CHECK(jpb_PCXInputConnectedCount(&xinput) == 2);
    CHECK(xinput.connectedMask == UINT32_C(0x0c));
    CHECK(jpb_PCControllerUserForPlayer(
        0, 2, 3, xinput.connectedMask, &player_one_user));
    CHECK(jpb_PCControllerUserForPlayer(
        1, 2, 3, xinput.connectedMask, &player_two_user));
    CHECK(player_one_user == 2);
    CHECK(player_two_user == 3);

    CHECK(jpb_PCXInputReadUser(
        &xinput, player_one_user, 2, 8, 0, 0,
        &bits, &axis_x, &axis_y));
    CHECK(bits == JPB_PAD_COMBO_SOUTH);
    CHECK(axis_x == 0.0f && axis_y == 0.0f);
    CHECK(jpb_PCXInputReadUser(
        &xinput, player_two_user, 2, 8, 0, 0,
        &bits, &axis_x, &axis_y));
    CHECK(bits == JPB_PAD_JUMP);
    CHECK(!jpb_PCXInputReadUser(
        &xinput, 0, 2, 8, 0, 0,
        &bits, &axis_x, &axis_y));
    CHECK(!jpb_PCXInputReadUser(
        &xinput, 4, 2, 8, 0, 0,
        &bits, &axis_x, &axis_y));

    jpb_PCXInputSetRumbleUser(&xinput, 3, 123, 456, 100);
    CHECK(fake_xinput.setCalls[3] == 1);
    CHECK(fake_xinput.lastVibration[3].leftMotorSpeed == 123);
    CHECK(fake_xinput.lastVibration[3].rightMotorSpeed == 456);
    CHECK(xinput.rumbleStopAt[3] != 0);
    jpb_PCXInputSetRumbleUser(&xinput, 4, 1, 1, 1);
    CHECK(fake_xinput.setCalls[3] == 1);

    jpb_PCXInputShutdown(&xinput);
    CHECK(fake_xinput.setCalls[2] == 1);
    CHECK(fake_xinput.setCalls[3] == 2);
    CHECK(fake_xinput.lastVibration[2].leftMotorSpeed == 0);
    CHECK(fake_xinput.lastVibration[2].rightMotorSpeed == 0);
    CHECK(fake_xinput.lastVibration[3].leftMotorSpeed == 0);
    CHECK(fake_xinput.lastVibration[3].rightMotorSpeed == 0);
    CHECK(xinput.getState == NULL);
    CHECK(xinput.setState == NULL);
    return 0;
}

int main(void)
{
    CHECK(sizeof(JPBPCXInputGamepad) == 12);
    CHECK(sizeof(JPBPCXInputStatePacket) == 16);
    CHECK(sizeof(JPBPCXInputVibration) == 4);
    CHECK(test_exact_button_layout() == 0);
    CHECK(test_directions_and_thresholds() == 0);
    CHECK(test_exact_gameplay_keyboard_layout() == 0);
    CHECK(test_exact_menu_keyboard_layout() == 0);
    CHECK(test_controller_ownership() == 0);
    CHECK(test_sparse_physical_user_routing() == 0);
    puts("PC XInput tests passed");
    return 0;
}
