#include "pc_xinput_win32.h"

#include "jpb/input.h"

#include <string.h>

enum { JPB_PC_XINPUT_MAX_USERS = 4 };

uint32_t jpb_PCXInputMapGamepad(
    const JPBPCXInputGamepad *gamepad,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu,
    float *axis_x,
    float *axis_y)
{
    JPBControllerState controller;

    if (gamepad == NULL) {
        if (axis_x != NULL) *axis_x = 0.0f;
        if (axis_y != NULL) *axis_y = 0.0f;
        return 0;
    }
    memset(&controller, 0, sizeof(controller));
    controller.name = "Xbox Series X Controller";
    controller.axis[0] = gamepad->thumbLX;
    controller.axis[1] = -(int32_t)gamepad->thumbLY;
    controller.axis[4] = gamepad->leftTrigger;
    controller.axis[5] = gamepad->rightTrigger;
    controller.button[0] = (gamepad->buttons & JPB_PC_XINPUT_A) != 0;
    controller.button[1] = (gamepad->buttons & JPB_PC_XINPUT_B) != 0;
    controller.button[2] = (gamepad->buttons & JPB_PC_XINPUT_X) != 0;
    controller.button[3] = (gamepad->buttons & JPB_PC_XINPUT_Y) != 0;
    controller.button[4] = (gamepad->buttons & JPB_PC_XINPUT_BACK) != 0;
    controller.button[6] = (gamepad->buttons & JPB_PC_XINPUT_START) != 0;
    controller.button[9] =
        (gamepad->buttons & JPB_PC_XINPUT_LEFT_SHOULDER) != 0;
    controller.button[10] =
        (gamepad->buttons & JPB_PC_XINPUT_RIGHT_SHOULDER) != 0;
    controller.button[11] =
        (gamepad->buttons & JPB_PC_XINPUT_DPAD_UP) != 0;
    controller.button[12] =
        (gamepad->buttons & JPB_PC_XINPUT_DPAD_DOWN) != 0;
    controller.button[13] =
        (gamepad->buttons & JPB_PC_XINPUT_DPAD_LEFT) != 0;
    controller.button[14] =
        (gamepad->buttons & JPB_PC_XINPUT_DPAD_RIGHT) != 0;
    return jpb_InputMapControllerState(
        &controller,
        walk_limit,
        run_limit,
        controller_config,
        in_menu,
        axis_x,
        axis_y);
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
