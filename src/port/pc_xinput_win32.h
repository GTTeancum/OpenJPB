#ifndef JPB_PC_XINPUT_WIN32_H
#define JPB_PC_XINPUT_WIN32_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

/*
 * Minimal XInput ABI used through GetProcAddress. Keeping the declarations
 * here avoids both an xinput import library and a bundled controller runtime.
 */
typedef struct JPBPCXInputGamepad {
    uint16_t buttons;
    uint8_t leftTrigger;
    uint8_t rightTrigger;
    int16_t thumbLX;
    int16_t thumbLY;
    int16_t thumbRX;
    int16_t thumbRY;
} JPBPCXInputGamepad;

typedef struct JPBPCXInputStatePacket {
    uint32_t packetNumber;
    JPBPCXInputGamepad gamepad;
} JPBPCXInputStatePacket;

typedef struct JPBPCXInputVibration {
    uint16_t leftMotorSpeed;
    uint16_t rightMotorSpeed;
} JPBPCXInputVibration;

typedef DWORD (WINAPI *JPBPCXInputGetStateProc)(
    DWORD user_index, JPBPCXInputStatePacket *state);
typedef DWORD (WINAPI *JPBPCXInputSetStateProc)(
    DWORD user_index, JPBPCXInputVibration *vibration);

typedef struct JPBPCXInput {
    HMODULE module;
    JPBPCXInputGetStateProc getState;
    JPBPCXInputSetStateProc setState;
    uint32_t connectedMask;
    uint64_t rumbleStopAt[4];
} JPBPCXInput;

enum {
    JPB_PC_XINPUT_DPAD_UP = 0x0001,
    JPB_PC_XINPUT_DPAD_DOWN = 0x0002,
    JPB_PC_XINPUT_DPAD_LEFT = 0x0004,
    JPB_PC_XINPUT_DPAD_RIGHT = 0x0008,
    JPB_PC_XINPUT_START = 0x0010,
    JPB_PC_XINPUT_BACK = 0x0020,
    JPB_PC_XINPUT_LEFT_THUMB = 0x0040,
    JPB_PC_XINPUT_RIGHT_THUMB = 0x0080,
    JPB_PC_XINPUT_LEFT_SHOULDER = 0x0100,
    JPB_PC_XINPUT_RIGHT_SHOULDER = 0x0200,
    JPB_PC_XINPUT_A = 0x1000,
    JPB_PC_XINPUT_B = 0x2000,
    JPB_PC_XINPUT_X = 0x4000,
    JPB_PC_XINPUT_Y = 0x8000
};

int jpb_PCXInputInit(JPBPCXInput *xinput);
void jpb_PCXInputShutdown(JPBPCXInput *xinput);
unsigned jpb_PCXInputConnectedCount(JPBPCXInput *xinput);
/* Read and drive an exact physical XInput user slot. Device ownership is
 * resolved separately by the recovered controller-assignment layer. */
int jpb_PCXInputReadUser(
    JPBPCXInput *xinput,
    unsigned user_index,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu,
    uint32_t *pad_bits,
    float *axis_x,
    float *axis_y);
void jpb_PCXInputSetRumbleUser(
    JPBPCXInput *xinput,
    unsigned user_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms);

/* Pure recovered-layout translation, exposed for host regression tests. */
uint32_t jpb_PCXInputMapGamepad(
    const JPBPCXInputGamepad *gamepad,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu,
    float *axis_x,
    float *axis_y);

#endif
