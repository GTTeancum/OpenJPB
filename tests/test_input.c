#include "jpb/input.h"
#include "jpb/game.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr, "CHECK failed at %s:%d: %s\n",                       \
                __FILE__, __LINE__, #condition);                             \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct TestInput {
    uint32_t pads[JPB_INPUT_PAD_COUNT];
    int lastPad;
    int reads;
} TestInput;

typedef struct TestRumble {
    int calls;
    int controller;
    uint16_t low;
    uint16_t high;
    uint32_t duration;
} TestRumble;

static void record_rumble(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data)
{
    TestRumble *rumble = (TestRumble *)user_data;

    ++rumble->calls;
    rumble->controller = controller_index;
    rumble->low = low_frequency;
    rumble->high = high_frequency;
    rumble->duration = duration_ms;
}

static uint32_t read_test_pad(int32_t pad_index, void *user_data)
{
    TestInput *input = (TestInput *)user_data;

    input->lastPad = pad_index;
    ++input->reads;
    return input->pads[pad_index];
}

static int test_clear_and_init(void)
{
    size_t index;

    memset(padMaskBits, 0xff, sizeof(padMaskBits));
    memset(padCurrentBits, 0xff, sizeof(padCurrentBits));
    ClearInput();
    for (index = 0; index < JPB_INPUT_PAD_COUNT; ++index) {
        CHECK(padMaskBits[index] == 0);
        CHECK(padCurrentBits[index].padLevel1 == 0);
        CHECK(padCurrentBits[index].padLevel2 == 0);
    }

    nShockers[0] = 10;
    nShockers[1] = 11;
    padExist = 0xff;
    initPSXPad();
    CHECK(nShockers[0] == 0);
    CHECK(nShockers[1] == 0);
    CHECK(padExist == 0);
    return 0;
}

static int test_controller_packet_lifecycle(void)
{
    TestRumble rumble = {0, -1, 0, 0, 0};

    memset(padBuffer, 0, sizeof(padBuffer));
    padTypes = 0;
    padExist = 0;
    padShockable = 0;

    padBuffer[0].dataFormat = (uint8_t)'s';
    CHECK(PadGone(0) == 0);
    CHECK((padExist & 1u) != 0);
    CHECK((padTypes & 1u) != 0);

    padBuffer[0].dataFormat = (uint8_t)'A';
    CHECK(PadGone(0) == 0);
    CHECK((padExist & 1u) != 0);
    CHECK((padTypes & 1u) == 0);

    jpb_InputSetRumbleProvider(record_rumble, &rumble);
    padTypes |= 1u;
    padShockable |= 1u;
    padBuffer[0].transStatus = 1;
    CHECK(PadGone(0) == 1);
    CHECK((padExist & 1u) == 0);
    CHECK((padTypes & 1u) == 0);
    CHECK((padShockable & 1u) == 0);
    CHECK(rumble.calls == 1);
    CHECK(rumble.controller == 0);
    CHECK(rumble.low == 0 && rumble.high == 0);

    padBuffer[1].transStatus = 0;
    padBuffer[1].dataFormat = 0xff;
    CHECK(PadGone(1) == 1);
    CHECK((padExist & 2u) == 0);
    CHECK(rumble.calls == 1);

    jpb_InputSetRumbleProvider(NULL, NULL);
    return 0;
}

static int test_windows_input_state_owners(void)
{
    p1Disconnected = 7;
    p2Disconnected = 8;
    p2Connected = 9;
    P1Disconnected();
    CHECK(p1Disconnected == 7);
    CHECK(p2Disconnected == 8);
    CHECK(p2Connected == 9);

    P2Disconnected();
    CHECK(p1Disconnected == 7);
    CHECK(p2Disconnected == 1);
    CHECK(p2Connected == 0);

    padCurrentBits[1].padLevel1 = UINT32_C(1) << 7;
    CHECK(padbuttonpressed(1, 7) != 0);
    CHECK(padbuttonpressed(1, 6) == 0);

    inMenuState = -1;
    inTitleState = -1;
    setMenuBindings(3, 5);
    CHECK(inMenuState == 3);
    CHECK(inTitleState == 5);
    return 0;
}

static int test_edge_and_continuous_masks(void)
{
    TestInput input = {{0}, -1, 0};
    uint32_t *oldBits = &padCurrentBits[0].padLevel1;

    ClearInput();
    jpb_InputSetProvider(read_test_pad, &input);
    GameStruct.inMenuFlag = 1;
    GameStruct.gameMode = 6;

    input.pads[0] = JPB_PAD_UP;
    CHECK(input_ReadControlPad(0, 0, oldBits) == 0);
    CHECK(inMenuState == 1);
    CHECK(inTitleState == 6);
    CHECK(*oldBits == 0);
    CHECK(input.lastPad == 0);

    input.pads[0] = 0;
    CHECK(input_ReadControlPad(0, 0, oldBits) == 0);
    CHECK(padMaskBits[0] == UINT32_MAX);

    input.pads[0] = JPB_PAD_UP | JPB_PAD_RIGHT;
    CHECK(input_ReadControlPad(0, 0, oldBits) ==
          (JPB_PAD_UP | JPB_PAD_RIGHT));
    CHECK(*oldBits == (JPB_PAD_UP | JPB_PAD_RIGHT));

    CHECK(input_ReadControlPad(0, 0, oldBits) == 0);
    CHECK(input_ReadControlPad(
              0, JPB_PAD_UP | JPB_PAD_RIGHT, oldBits) ==
          (JPB_PAD_UP | JPB_PAD_RIGHT));

    input.pads[0] = JPB_PAD_RIGHT;
    CHECK(input_ReadControlPad(
              0, JPB_PAD_UP | JPB_PAD_RIGHT, oldBits) ==
          JPB_PAD_RIGHT);

    input.pads[0] = 0;
    CHECK(input_ReadControlPad(0, 0, oldBits) == 0);
    input.pads[0] = JPB_PAD_LEFT;
    CHECK(input_ReadControlPad(0x8000, 0, oldBits) == JPB_PAD_LEFT);
    CHECK(input.lastPad == 0);
    CHECK(input.reads == 8);
    return 0;
}

static int test_mask_held_buttons(void)
{
    TestInput input = {{0}, -1, 0};
    uint32_t oldBits = 0;

    ClearInput();
    jpb_InputSetProvider(read_test_pad, &input);
    GameStruct.inMenuFlag = 0;
    GameStruct.gameMode = 3;
    input.pads[1] = JPB_PAD_DOWN;
    maskPadBits(1);
    CHECK(inMenuState == 0);
    CHECK(inTitleState == 3);
    CHECK(padMaskBits[1] == 0);
    CHECK(input_ReadControlPad(1, JPB_PAD_DOWN, &oldBits) == 0);

    input.pads[1] = 0;
    CHECK(input_ReadControlPad(1, 0, &oldBits) == 0);
    input.pads[1] = JPB_PAD_DOWN;
    CHECK(input_ReadControlPad(1, 0, &oldBits) == JPB_PAD_DOWN);

    jpb_InputSetProvider(NULL, NULL);
    CHECK(jpb_InputReadRawPad(0) == 0);
    return 0;
}

static int test_feedback_effects(void)
{
    TestRumble rumble = {0, -1, 0, 0, 0};

    memset(&OptionStruct, 0, sizeof(OptionStruct));
    jpb_InputSetRumbleProvider(
        record_rumble, &rumble);
    feedback_startEffect(0, 13);
    CHECK(rumble.calls == 0);

    OptionStruct.ShockFlag[0] = 1;
    feedback_startEffect(0, 13);
    CHECK(rumble.calls == 1);
    CHECK(rumble.controller == 0);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 20560);
    CHECK(rumble.duration == 300);

    feedback_startEffect(0, 15);
    CHECK(rumble.calls == 1);
    vibration_stop(0);
    CHECK(rumble.calls == 2);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 0);
    CHECK(rumble.duration == 0);
    jpb_InputSetRumbleProvider(NULL, NULL);
    return 0;
}

static int test_feedback_lifecycle(void)
{
    TestRumble rumble = {0, -1, 0, 0, 0};
    int frame;

    memset(&OptionStruct, 0, sizeof(OptionStruct));
    initPSXPad();
    nShockers[0] = 2;
    OptionStruct.ShockFlag[0] = 1;
    jpb_InputSetRumbleProvider(record_rumble, &rumble);

    feedback_startEffect(0, 3);
    CHECK(rumble.calls == 1);
    CHECK(rumble.controller == 0);
    CHECK(rumble.low == 65535);
    CHECK(rumble.high == 16448);
    CHECK(rumble.duration == 400);

    for (frame = 0; frame < 3; ++frame) {
        psxUpdatePadbits();
        CHECK(rumble.calls == 1);
    }
    psxUpdatePadbits();
    CHECK(rumble.calls == 2);
    CHECK(rumble.controller == 0);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 0);
    CHECK(rumble.duration == 0);

    feedback_startEffect(0, 3);
    CHECK(rumble.calls == 3);
    clearShockers(0, 0);
    CHECK(rumble.calls == 4);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 0);
    CHECK(rumble.duration == 0);

    jpb_InputSetRumbleProvider(NULL, NULL);
    return 0;
}

static int test_start_rumble(void)
{
    TestRumble rumble = {0, -1, 0, 0, 0};

    memset(&OptionStruct, 0, sizeof(OptionStruct));
    initPSXPad();
    jpb_InputSetRumbleProvider(record_rumble, &rumble);
    startRumble(1, 1234);
    CHECK(nShockers[1] == 1);
    CHECK(rumble.calls == 0);
    nShockers[1] = 0;

    OptionStruct.ShockFlag[0] = 1;
    startRumble(0, 1234);
    CHECK(nShockers[0] == 1);
    CHECK(rumble.calls == 1);
    CHECK(rumble.controller == 0);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 16448);
    CHECK(rumble.duration == 200);

    psxUpdatePadbits();
    CHECK(rumble.calls == 1);
    psxUpdatePadbits();
    CHECK(rumble.calls == 2);
    CHECK(rumble.low == 0);
    CHECK(rumble.high == 0);
    CHECK(rumble.duration == 0);

    jpb_InputSetRumbleProvider(NULL, NULL);
    return 0;
}

int main(void)
{
    CHECK(test_clear_and_init() == 0);
    CHECK(test_controller_packet_lifecycle() == 0);
    CHECK(test_windows_input_state_owners() == 0);
    CHECK(test_edge_and_continuous_masks() == 0);
    CHECK(test_mask_held_buttons() == 0);
    CHECK(test_feedback_effects() == 0);
    CHECK(test_feedback_lifecycle() == 0);
    CHECK(test_start_rumble() == 0);
    puts("input tests passed");
    return 0;
}
