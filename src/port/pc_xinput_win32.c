#include "pc_xinput_win32.h"

#include "jpb/input.h"

#include <math.h>
#include <string.h>

enum {
    JPB_PC_XINPUT_MAX_USERS = 4,
    JPB_PC_XINPUT_LEFT_THUMB_DEADZONE = 7849
};

static void jpb_pc_xinput_condition_left_stick(
    JPBPCXInputGamepad *gamepad)
{
    float x;
    float y;
    float magnitude;
    float normalized_magnitude;
    float scale;

    if (gamepad == NULL) {
        return;
    }
    x = (float)gamepad->thumbLX;
    y = (float)gamepad->thumbLY;
    magnitude = sqrtf(x * x + y * y);
    if (magnitude <= (float)JPB_PC_XINPUT_LEFT_THUMB_DEADZONE) {
        gamepad->thumbLX = 0;
        gamepad->thumbLY = 0;
        return;
    }

    if (magnitude > 32767.0f) {
        magnitude = 32767.0f;
    }
    normalized_magnitude =
        (magnitude - (float)JPB_PC_XINPUT_LEFT_THUMB_DEADZONE) /
        (32767.0f - (float)JPB_PC_XINPUT_LEFT_THUMB_DEADZONE);
    scale = normalized_magnitude * 32767.0f /
            sqrtf(x * x + y * y);
    gamepad->thumbLX = (int16_t)(x * scale);
    gamepad->thumbLY = (int16_t)(y * scale);
}

static float jpb_pc_xinput_limit(uint8_t percentage)
{
    if (percentage > 100u) {
        percentage = 100u;
    }
    return (float)percentage / 100.0f;
}

uint32_t jpb_PCXInputMapGamepad(
    const JPBPCXInputGamepad *gamepad,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu,
    float *axis_x,
    float *axis_y)
{
    uint32_t bits = 0;
    float x;
    float y;
    float raw_x;
    float raw_y;
    float walk_threshold;
    float run_threshold;

    if (axis_x != NULL) {
        *axis_x = 0.0f;
    }
    if (axis_y != NULL) {
        *axis_y = 0.0f;
    }
    if (gamepad == NULL) {
        return 0;
    }

    /* ReadJoystickInput divides SDL's signed 16-bit axes by RAND_MAX. This
     * deliberately leaves -32768 slightly below -1 while +32767 is exact. */
    raw_x = (float)gamepad->thumbLX / 32767.0f;
    /* SDL axis 1 is positive down; XInput thumb Y is positive up. */
    raw_y = -(float)gamepad->thumbLY / 32767.0f;
    x = raw_x;
    y = raw_y;
    walk_threshold = jpb_pc_xinput_limit(walk_limit);
    run_threshold = jpb_pc_xinput_limit(run_limit);

    if (raw_x < -walk_threshold) bits |= JPB_PAD_RIGHT;
    if (raw_x > walk_threshold) bits |= JPB_PAD_LEFT;
    if (raw_y < -walk_threshold) bits |= JPB_PAD_UP;
    if (raw_y > walk_threshold) bits |= JPB_PAD_DOWN;
    if ((bits & (JPB_PAD_UP | JPB_PAD_LEFT |
                 JPB_PAD_DOWN | JPB_PAD_RIGHT)) == 0) {
        x = 0.0f;
        y = 0.0f;
    }
    if ((bits & (JPB_PAD_UP | JPB_PAD_LEFT |
                 JPB_PAD_DOWN | JPB_PAD_RIGHT)) != 0 &&
        sqrtf(raw_x * raw_x + raw_y * raw_y) > run_threshold) {
        bits |= JPB_PAD_ANALOG_MOVEMENT;
    }

    if ((gamepad->buttons & JPB_PC_XINPUT_DPAD_UP) != 0) {
        bits |= JPB_PAD_UP;
        y -= 1.0f;
    }
    if ((gamepad->buttons & JPB_PC_XINPUT_DPAD_DOWN) != 0) {
        bits |= JPB_PAD_DOWN;
        y += 1.0f;
    }
    if ((gamepad->buttons & JPB_PC_XINPUT_DPAD_LEFT) != 0) {
        bits |= JPB_PAD_RIGHT;
        x -= 1.0f;
    }
    if ((gamepad->buttons & JPB_PC_XINPUT_DPAD_RIGHT) != 0) {
        bits |= JPB_PAD_LEFT;
        x += 1.0f;
    }

    /* Exact generic-controller branches from ReadJoystickInput. Menus always
     * use the classic A-confirm/B-back arrangement; gameplay configuration
     * one swaps those actions to the shipped modern layout. */
    if (controller_config == 0 || in_menu) {
        if ((gamepad->buttons & JPB_PC_XINPUT_A) != 0)
            bits |= JPB_PAD_COMBO_SOUTH;
        if ((gamepad->buttons & JPB_PC_XINPUT_B) != 0)
            bits |= JPB_PAD_JUMP;
        if ((gamepad->buttons & JPB_PC_XINPUT_Y) != 0)
            bits |= JPB_PAD_COMBO_NORTH;
    } else {
        if ((gamepad->buttons & JPB_PC_XINPUT_A) != 0)
            bits |= JPB_PAD_JUMP;
        if ((gamepad->buttons & JPB_PC_XINPUT_B) != 0)
            bits |= JPB_PAD_COMBO_NORTH;
        if ((gamepad->buttons & JPB_PC_XINPUT_Y) != 0)
            bits |= JPB_PAD_COMBO_SOUTH;
    }
    if ((gamepad->buttons & JPB_PC_XINPUT_X) != 0)
        bits |= JPB_PAD_COMBO_WEST;
    if ((gamepad->buttons & JPB_PC_XINPUT_LEFT_SHOULDER) != 0)
        bits |= JPB_PAD_BLOCK;
    if ((gamepad->buttons & JPB_PC_XINPUT_RIGHT_SHOULDER) != 0)
        bits |= JPB_PAD_LOCK_ON;
    if ((gamepad->buttons & JPB_PC_XINPUT_BACK) != 0)
        bits |= JPB_PAD_ZOOM_IN;
    if ((gamepad->buttons & JPB_PC_XINPUT_START) != 0)
        bits |= JPB_PAD_START;
    if (gamepad->leftTrigger != 0)
        bits |= JPB_PAD_LEFT_TRIGGER;
    if (gamepad->rightTrigger != 0)
        bits |= JPB_PAD_RIGHT_TRIGGER;
    if (axis_x != NULL) *axis_x = x;
    if (axis_y != NULL) *axis_y = y;
    return bits;
}

static void jpb_pc_xinput_refresh(JPBPCXInput *xinput)
{
    JPBPCXInputStatePacket state;
    JPBPCXInputVibration stopped = {0, 0};
    uint64_t now;
    unsigned user;

    if (xinput == NULL || xinput->getState == NULL) {
        return;
    }
    now = GetTickCount64();
    xinput->connectedMask = 0;
    for (user = 0; user < JPB_PC_XINPUT_MAX_USERS; ++user) {
        memset(&state, 0, sizeof(state));
        if (xinput->getState(user, &state) == ERROR_SUCCESS) {
            xinput->connectedMask |= UINT32_C(1) << user;
            if (xinput->rumbleStopAt[user] != 0 &&
                now >= xinput->rumbleStopAt[user]) {
                if (xinput->setState != NULL) {
                    (void)xinput->setState(user, &stopped);
                }
                xinput->rumbleStopAt[user] = 0;
            }
        } else {
            xinput->rumbleStopAt[user] = 0;
        }
    }
}

int jpb_PCXInputInit(JPBPCXInput *xinput)
{
    static const char *const module_names[] = {
        "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll"
    };
    size_t index;

    if (xinput == NULL) {
        return 0;
    }
    memset(xinput, 0, sizeof(*xinput));
    for (index = 0; index < sizeof(module_names) / sizeof(module_names[0]);
         ++index) {
        xinput->module = LoadLibraryA(module_names[index]);
        if (xinput->module != NULL) {
            break;
        }
    }
    if (xinput->module == NULL) {
        return 0;
    }
    xinput->getState = (JPBPCXInputGetStateProc)(uintptr_t)GetProcAddress(
        xinput->module, "XInputGetState");
    xinput->setState = (JPBPCXInputSetStateProc)(uintptr_t)GetProcAddress(
        xinput->module, "XInputSetState");
    if (xinput->getState == NULL) {
        jpb_PCXInputShutdown(xinput);
        return 0;
    }
    jpb_pc_xinput_refresh(xinput);
    return 1;
}

void jpb_PCXInputShutdown(JPBPCXInput *xinput)
{
    unsigned user;

    if (xinput == NULL) {
        return;
    }
    for (user = 0; user < JPB_PC_XINPUT_MAX_USERS; ++user) {
        jpb_PCXInputSetRumbleUser(xinput, user, 0, 0, 0);
    }
    if (xinput->module != NULL) {
        FreeLibrary(xinput->module);
    }
    memset(xinput, 0, sizeof(*xinput));
}

unsigned jpb_PCXInputConnectedCount(JPBPCXInput *xinput)
{
    unsigned count = 0;
    unsigned user;

    jpb_pc_xinput_refresh(xinput);
    for (user = 0; user < JPB_PC_XINPUT_MAX_USERS; ++user) {
        if ((xinput->connectedMask & (UINT32_C(1) << user)) != 0) {
            ++count;
        }
    }
    return count;
}

int jpb_PCXInputReadUser(
    JPBPCXInput *xinput,
    unsigned user_index,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu,
    uint32_t *pad_bits,
    float *axis_x,
    float *axis_y)
{
    JPBPCXInputStatePacket state;

    if (pad_bits != NULL) *pad_bits = 0;
    if (axis_x != NULL) *axis_x = 0.0f;
    if (axis_y != NULL) *axis_y = 0.0f;
    if (xinput == NULL || xinput->getState == NULL ||
        user_index >= JPB_PC_XINPUT_MAX_USERS) {
        return 0;
    }
    jpb_pc_xinput_refresh(xinput);
    if ((xinput->connectedMask &
         (UINT32_C(1) << user_index)) == 0) {
        return 0;
    }
    memset(&state, 0, sizeof(state));
    if (xinput->getState(user_index, &state) != ERROR_SUCCESS) {
        xinput->connectedMask &= ~(UINT32_C(1) << user_index);
        return 0;
    }
    /* XInput reports raw stick values and defines this deadzone for the left
     * thumbstick. Condition at the backend boundary before applying the
     * recovered game's authored walk/run thresholds. */
    jpb_pc_xinput_condition_left_stick(&state.gamepad);
    if (pad_bits != NULL) {
        *pad_bits = jpb_PCXInputMapGamepad(
            &state.gamepad,
            walk_limit,
            run_limit,
            controller_config,
            in_menu,
            axis_x,
            axis_y);
    }
    return 1;
}

void jpb_PCXInputSetRumbleUser(
    JPBPCXInput *xinput,
    unsigned user_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms)
{
    JPBPCXInputVibration vibration;

    if (xinput == NULL || xinput->setState == NULL ||
        user_index >= JPB_PC_XINPUT_MAX_USERS) {
        return;
    }
    jpb_pc_xinput_refresh(xinput);
    if ((xinput->connectedMask &
         (UINT32_C(1) << user_index)) == 0) {
        return;
    }
    vibration.leftMotorSpeed = low_frequency;
    vibration.rightMotorSpeed = high_frequency;
    if (xinput->setState(user_index, &vibration) == ERROR_SUCCESS) {
        xinput->rumbleStopAt[user_index] =
            duration_ms != 0 && (low_frequency != 0 || high_frequency != 0)
                ? GetTickCount64() + duration_ms
                : 0;
    }
}
