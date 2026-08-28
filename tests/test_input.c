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

static int read_joystick_count(void *user_data)
{
    return *(const int *)user_data;
}

static const JPBKeyboardState *read_keyboard_state(void *user_data)
{
    return (const JPBKeyboardState *)user_data;
}

typedef struct TestControllerInput {
    JPBControllerState state;
    int calls;
} TestControllerInput;

typedef struct TestControllerDevices {
    SDL_GameController *opened[8];
    SDL_GameController *detached[5];
    int detachedCount;
    int openCalls;
    int lastDevice;
    int attachedCalls;
} TestControllerDevices;

typedef struct TestInputEvents {
    JPBInputEvent events[8];
    SDL_GameController *controllers[8];
    int eventCount;
    int eventIndex;
    int lookupCalls;
} TestInputEvents;

typedef struct TestDirectInputDevice {
    JPBDirectInputDevice device;
    JPBDirectInputDevice *queryResult;
    JPBHRESULT queryStatus;
    JPBHRESULT setPropertyStatus;
    JPBHRESULT acquireStatus;
    int queryCalls;
    int releaseCalls;
    int acquireCalls;
    int unacquireCalls;
    int setPropertyCalls;
    const JPB_GUID *lastIid;
    const JPB_GUID *lastProperty;
    JPBDIPropertyDword property;
} TestDirectInputDevice;

static int read_controller_state(
    SDL_GameController *controller,
    JPBControllerState *state,
    void *user_data)
{
    TestControllerInput *input = (TestControllerInput *)user_data;

    (void)controller;
    ++input->calls;
    *state = input->state;
    return 1;
}

static SDL_GameController *open_controller_device(
    int device,
    void *user_data)
{
    TestControllerDevices *devices = (TestControllerDevices *)user_data;

    ++devices->openCalls;
    devices->lastDevice = device;
    return devices->opened[device];
}

static int controller_device_attached(
    SDL_GameController *controller,
    void *user_data)
{
    TestControllerDevices *devices = (TestControllerDevices *)user_data;
    int index;

    ++devices->attachedCalls;
    for (index = 0; index < devices->detachedCount; ++index) {
        if (devices->detached[index] == controller) {
            return 0;
        }
    }
    return 1;
}

static int poll_input_event(JPBInputEvent *event, void *user_data)
{
    TestInputEvents *events = (TestInputEvents *)user_data;

    if (events->eventIndex == events->eventCount) {
        return 0;
    }
    *event = events->events[events->eventIndex++];
    return 1;
}

static SDL_GameController *lookup_event_controller(
    int32_t instance_id,
    void *user_data)
{
    TestInputEvents *events = (TestInputEvents *)user_data;

    ++events->lookupCalls;
    return events->controllers[instance_id];
}

static JPBHRESULT direct_input_query(
    JPBDirectInputDevice *device,
    const JPB_GUID *iid,
    void **result)
{
    TestDirectInputDevice *test = (TestDirectInputDevice *)device;

    ++test->queryCalls;
    test->lastIid = iid;
    *result = test->queryResult;
    return test->queryStatus;
}

static uint32_t direct_input_release(JPBDirectInputDevice *device)
{
    TestDirectInputDevice *test = (TestDirectInputDevice *)device;

    ++test->releaseCalls;
    return (uint32_t)test->releaseCalls;
}

static JPBHRESULT direct_input_set_property(
    JPBDirectInputDevice *device,
    const JPB_GUID *property,
    const JPBDIPropertyHeader *header)
{
    TestDirectInputDevice *test = (TestDirectInputDevice *)device;

    ++test->setPropertyCalls;
    test->lastProperty = property;
    test->property = *(const JPBDIPropertyDword *)header;
    return test->setPropertyStatus;
}

static JPBHRESULT direct_input_acquire(JPBDirectInputDevice *device)
{
    TestDirectInputDevice *test = (TestDirectInputDevice *)device;

    ++test->acquireCalls;
    return test->acquireStatus;
}

static JPBHRESULT direct_input_unacquire(JPBDirectInputDevice *device)
{
    TestDirectInputDevice *test = (TestDirectInputDevice *)device;

    ++test->unacquireCalls;
    return 0;
}

static int test_keyboard_owner(void)
{
    JPBKeyboardState keyboard;

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.moveUp = 1;
    keyboard.moveLeft = 1;
    keyboard.walkModifier = 1;
    inMenuState = 0;
    inTitleState = 0;
    mini2_keyboardOverride = 0;
    jpb_InputSetKeyboardStateProvider(read_keyboard_state, &keyboard);
    CHECK(ReadKeyboardInput() ==
          (JPB_PAD_UP | JPB_PAD_RIGHT | JPB_PAD_ANALOG_MOVEMENT));
    CHECK(g_p1X == -1.0f);
    CHECK(g_p1Y == -1.0f);

    memset(&keyboard, 0, sizeof(keyboard));
    keyboard.enter = 1;
    CHECK(ReadKeyboardInput() ==
          (JPB_PAD_ZOOM_IN | JPB_PAD_JUMP));
    mini2_keyboardOverride = 1;
    CHECK(ReadKeyboardInput() ==
          (JPB_PAD_ZOOM_IN | JPB_PAD_JUMP));
    keyboard.enter = 0;
    keyboard.space = 1;
    CHECK(ReadKeyboardInput() ==
          (JPB_PAD_ZOOM_IN | JPB_PAD_JUMP | JPB_PAD_COMBO_NORTH));

    inMenuState = 1;
    inTitleState = 0;
    CHECK(ReadKeyboardInput() == JPB_PAD_START);
    inTitleState = 1;
    CHECK(ReadKeyboardInput() == JPB_PAD_COMBO_SOUTH);

    jpb_InputSetKeyboardStateProvider(NULL, NULL);
    CHECK(ReadKeyboardInput() == 0);
    mini2_keyboardOverride = 0;
    return 0;
}

static int test_joystick_owner(void)
{
    JPBKeyboardState keyboard;
    TestControllerInput controller;
    SDL_GameController *const handle =
        (SDL_GameController *)(uintptr_t)0x3333u;
    int joystick_count = 1;
    uint32_t bits;
    float axis_x;
    float axis_y;

    memset(&keyboard, 0, sizeof(keyboard));
    memset(&controller, 0, sizeof(controller));
    controller.state.name = "Xbox Series X Controller";
    controller.state.axis[0] = 12000;
    controller.state.button[0] = 1;
    sdlPads[0] = handle;
    gGameControllers[0] = NULL;
    gGameControllers[1] = NULL;
    inMenuState = 0;
    jpb_InputSetKeyboardStateProvider(read_keyboard_state, &keyboard);
    jpb_InputSetControllerStateProvider(
        read_controller_state, &controller);
    jpb_InputSetJoystickCountProvider(
        read_joystick_count, &joystick_count);

    bits = ReadJoystickInput(0, 2, 8, 0);
    CHECK(bits == (JPB_PAD_LEFT | JPB_PAD_COMBO_SOUTH));
    CHECK(controller.calls == 1);
    CHECK(gGameControllers[0] == handle);
    CHECK(player1InputType == 1);
    CHECK(lastUsedInputType == 1);
    CHECK(g_p1X > 0.366f && g_p1X < 0.367f);
    CHECK(JOYSTICK_DEAD_ZONE > 0.2834f);
    CHECK(JOYSTICK_DEAD_ZONE < 0.2835f);
    CHECK(JOYSTICK_RUN_LIMIT > 0.8346f);
    CHECK(JOYSTICK_RUN_LIMIT < 0.8347f);

    keyboard.escape = 1;
    bits = ReadJoystickInput(0, 2, 8, 0);
    CHECK(bits == JPB_PAD_START);
    CHECK(controller.calls == 1);
    CHECK(gGameControllers[0] == NULL);
    CHECK(player1InputType == 0);
    CHECK(lastUsedInputType == 0);
    keyboard.escape = 0;

    gGameControllers[1] = handle;
    controller.state.axis[0] = 0;
    controller.state.button[0] = 0;
    controller.state.button[1] = 1;
    bits = ReadJoystickInput(1, 2, 8, 0);
    CHECK(bits == JPB_PAD_JUMP);
    CHECK(controller.calls == 2);

    memset(&controller.state, 0, sizeof(controller.state));
    controller.state.name = "PS5 Controller";
    controller.state.button[20] = 1;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, NULL, NULL) ==
          JPB_PAD_ZOOM_IN);
    controller.state.name = "Nintendo Switch Pro Controller";
    controller.state.button[0] = 1;
    controller.state.button[20] = 0;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, NULL, NULL) ==
          JPB_PAD_JUMP);

    memset(&controller.state, 0, sizeof(controller.state));
    controller.state.name = "Xbox Series X Controller";
    controller.state.axis[0] = 7000;
    controller.state.axis[1] = 7000;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 0, 0, 0, NULL, NULL) ==
          JPB_PAD_ANALOG_MOVEMENT);

    controller.state.axis[0] = 0;
    controller.state.axis[1] = -9000;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, &axis_x, &axis_y) == 0);
    CHECK(axis_x == 0.0f);
    CHECK(axis_y == 0.0f);
    controller.state.axis[1] = -10000;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, &axis_x, &axis_y) ==
          JPB_PAD_UP);
    CHECK(axis_y < -0.3051f && axis_y > -0.3053f);
    controller.state.axis[1] = -27000;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, NULL, NULL) == JPB_PAD_UP);
    controller.state.axis[1] = -28000;
    CHECK(jpb_InputMapControllerState(
              &controller.state, 2, 8, 0, 0, NULL, NULL) ==
          (JPB_PAD_UP | JPB_PAD_ANALOG_MOVEMENT));

    sdlPads[0] = NULL;
    gGameControllers[1] = NULL;
    jpb_InputSetKeyboardStateProvider(NULL, NULL);
    jpb_InputSetControllerStateProvider(NULL, NULL);
    jpb_InputSetJoystickCountProvider(NULL, NULL);
    return 0;
}

static int test_controller_device_lifecycle(void)
{
    TestControllerDevices devices;
    SDL_GameController *const first =
        (SDL_GameController *)(uintptr_t)0x4100u;
    SDL_GameController *const second =
        (SDL_GameController *)(uintptr_t)0x4200u;
    SDL_GameController *const alternate =
        (SDL_GameController *)(uintptr_t)0x4300u;
    int index;

    memset(&devices, 0, sizeof(devices));
    memset(sdlPads, 0, sizeof(sdlPads));
    memset(gGameControllers, 0, sizeof(gGameControllers));
    devices.opened[3] = first;
    devices.opened[4] = second;
    devices.opened[5] = first;
    jpb_InputSetControllerDeviceProviders(
        open_controller_device, controller_device_attached, &devices);

    p2Disconnected = 0;
    AddControllerDevice(3);
    CHECK(devices.openCalls == 1);
    CHECK(devices.lastDevice == 3);
    CHECK(sdlPads[0] == first);
    CHECK(gGameControllers[0] == first);

    AddControllerDevice(4);
    CHECK(devices.openCalls == 2);
    CHECK(sdlPads[1] == second);
    CHECK(gGameControllers[0] == first);
    AddJoyDevice(7);
    CHECK(devices.openCalls == 2);

    for (index = 0; index < 5; ++index) {
        sdlPads[index] = first;
    }
    AddControllerDevice(4);
    CHECK(devices.openCalls == 2);

    memset(sdlPads, 0, sizeof(sdlPads));
    sdlPads[0] = first;
    sdlPads[1] = second;
    sdlPads[3] = alternate;
    gGameControllers[0] = first;
    gGameControllers[1] = NULL;
    p2Disconnected = 1;
    p2PadIndex = 2;
    lastUsedInputType = 9;
    AddControllerDevice(5);
    CHECK(devices.openCalls == 3);
    CHECK(p2Disconnected == 0);
    CHECK(sdlPads[2] == first);
    CHECK(gGameControllers[1] == first);
    CHECK(gGameControllers[0] == alternate);
    CHECK(lastUsedInputType == 1);

    devices.detached[0] = second;
    devices.detached[1] = first;
    devices.detachedCount = 2;
    UpdateJoyDevices();
    CHECK(sdlPads[0] == NULL);
    CHECK(sdlPads[1] == second);
    CHECK(sdlPads[2] == first);
    CHECK(p2Disconnected == 0);
    CHECK(gGameControllers[1] == first);
    UpdateJoyDevices();
    CHECK(sdlPads[1] == NULL);
    CHECK(sdlPads[2] == first);
    CHECK(p2Disconnected == 0);
    CHECK(gGameControllers[1] == first);
    UpdateJoyDevices();
    CHECK(sdlPads[2] == NULL);
    CHECK(p2Disconnected == 1);
    CHECK(gGameControllers[1] == NULL);

    memset(sdlPads, 0, sizeof(sdlPads));
    memset(gGameControllers, 0, sizeof(gGameControllers));
    p2PadIndex = -1;
    jpb_InputSetControllerDeviceProviders(NULL, NULL, NULL);
    return 0;
}

static int test_start_device_owner(void)
{
    JPBKeyboardState keyboard;
    TestControllerInput controller;
    SDL_GameController *const first =
        (SDL_GameController *)(uintptr_t)0x5100u;
    SDL_GameController *const second =
        (SDL_GameController *)(uintptr_t)0x5200u;
    int index;

    memset(&keyboard, 0, sizeof(keyboard));
    memset(&controller, 0, sizeof(controller));
    memset(sdlPads, 0, sizeof(sdlPads));
    controller.state.name = "PS5 Controller";
    controller.state.button[0] = 1;
    sdlPads[0] = first;
    gGameControllers[0] = NULL;
    gGameControllers[1] = second;
    padExist = 7;
    lastUsedInputType = 4;
    jpb_InputSetKeyboardStateProvider(read_keyboard_state, &keyboard);
    jpb_InputSetControllerStateProvider(
        read_controller_state, &controller);

    CHECK(checkStartDevice() == 1);
    CHECK(ReadGameInput == (JPBInputReadGameFunction)ReadJoystickInput);
    CHECK(padExist == 1);
    CHECK(gGameControllers[0] == first);
    CHECK(gGameControllers[1] == second);
    CHECK(lastUsedInputType == 4);

    memset(&controller.state, 0, sizeof(controller.state));
    controller.state.name = "Xbox Series X Controller";
    memset(sdlPads, 0, sizeof(sdlPads));
    sdlPads[0] = first;
    keyboard.space = 1;
    gGameControllers[0] = first;
    gGameControllers[1] = second;
    ReadGameInput = (JPBInputReadGameFunction)ReadJoystickInput;
    lastUsedInputType = 6;
    CHECK(checkStartDevice() == 1);
    CHECK(ReadGameInput == ReadKeyboardInput);
    CHECK(lastUsedInputType == 0);
    CHECK(gGameControllers[0] == NULL);
    CHECK(gGameControllers[1] == NULL);

    keyboard.space = 0;
    memset(&controller.state, 0, sizeof(controller.state));
    controller.state.name = "Nintendo Switch Pro Controller";
    controller.state.button[1] = 1;
    CHECK(checkStartDevice() == 1);
    CHECK(ReadGameInput == (JPBInputReadGameFunction)ReadJoystickInput);
    CHECK(gGameControllers[0] == first);

    memset(&controller.state, 0, sizeof(controller.state));
    keyboard.enter = 1;
    for (index = 0; index < 5; ++index) {
        sdlPads[index] = first;
    }
    gGameControllers[0] = second;
    ReadGameInput = (JPBInputReadGameFunction)ReadJoystickInput;
    CHECK(checkStartDevice() == 0);
    CHECK(ReadGameInput == (JPBInputReadGameFunction)ReadJoystickInput);
    CHECK(gGameControllers[0] == second);

    memset(sdlPads, 0, sizeof(sdlPads));
    memset(gGameControllers, 0, sizeof(gGameControllers));
    ReadGameInput = ReadKeyboardInput;
    jpb_InputSetKeyboardStateProvider(NULL, NULL);
    jpb_InputSetControllerStateProvider(NULL, NULL);
    return 0;
}

static int test_player_two_join_owner(void)
{
    TestControllerInput controller;
    TestInputEvents events;
    SDL_GameController *const first =
        (SDL_GameController *)(uintptr_t)0x6100u;
    SDL_GameController *const second =
        (SDL_GameController *)(uintptr_t)0x6200u;
    SDL_GameController *const alternate =
        (SDL_GameController *)(uintptr_t)0x6300u;

    memset(&controller, 0, sizeof(controller));
    memset(&events, 0, sizeof(events));
    memset(sdlPads, 0, sizeof(sdlPads));
    controller.state.name = "Xbox Series X Controller";
    events.controllers[0] = first;
    events.controllers[1] = second;
    jpb_InputSetControllerStateProvider(
        read_controller_state, &controller);
    jpb_InputSetEventProviders(
        poll_input_event, lookup_event_controller, &events);

    p2Connected = 7;
    p2PadIndex = 3;
    gGameControllers[1] = second;
    CHECK(checkP2Device() == 0);
    CHECK(p2Connected == 0);
    CHECK(p2PadIndex == -1);
    CHECK(gGameControllers[1] == NULL);

    events.events[0].type = UINT32_C(0x651);
    events.events[0].controllerInstanceId = 1;
    events.events[0].controllerButton = 0;
    events.eventCount = 1;
    events.eventIndex = 0;
    sdlPads[0] = first;
    sdlPads[1] = second;
    gGameControllers[0] = first;
    padExist = 0;
    CHECK(checkP2Device() == 1);
    CHECK(events.lookupCalls == 2);
    CHECK((padExist & 2u) != 0);
    CHECK(p2PadIndex == 1);
    CHECK(gGameControllers[1] == second);
    CHECK(p2Connected == 1);

    events.events[0].controllerButton = 1;
    events.eventIndex = 0;
    CHECK(checkP2Device() == -1);

    controller.state.name = "Nintendo Switch Pro Controller";
    events.events[0].controllerButton = 1;
    events.eventIndex = 0;
    CHECK(checkP2Device() == 1);
    CHECK(gGameControllers[1] == second);

    controller.state.name = "Xbox Series X Controller";
    events.events[0].controllerInstanceId = 0;
    events.events[0].controllerButton = 0;
    events.eventIndex = 0;
    memset(sdlPads, 0, sizeof(sdlPads));
    sdlPads[0] = first;
    sdlPads[2] = second;
    sdlPads[4] = alternate;
    gGameControllers[0] = first;
    gGameControllers[1] = NULL;
    padExist = 0;
    lastUsedInputType = 0;
    g_isSteamDeck = 0;
    CHECK(checkP2Device() == 1);
    CHECK(gGameControllers[0] == alternate);
    CHECK(gGameControllers[1] == first);
    CHECK(p2PadIndex == 0);
    CHECK(lastUsedInputType == 1);

    events.eventIndex = 0;
    gGameControllers[0] = first;
    gGameControllers[1] = second;
    p2Connected = 8;
    p2PadIndex = 4;
    padExist = 0;
    g_isSteamDeck = 1;
    CHECK(checkP2Device() == 0);
    CHECK(padExist == 0);
    CHECK(gGameControllers[0] == first);
    CHECK(gGameControllers[1] == second);
    CHECK(p2Connected == 8);
    CHECK(p2PadIndex == 4);

    memset(&events.events[0], 0, sizeof(events.events[0]));
    events.events[0].type = UINT32_C(0x300);
    events.events[0].keycode = 0x1b;
    events.eventIndex = 0;
    CHECK(checkP2Device() == -1);

    g_isSteamDeck = 0;
    memset(sdlPads, 0, sizeof(sdlPads));
    memset(gGameControllers, 0, sizeof(gGameControllers));
    p2Connected = 0;
    p2PadIndex = -1;
    jpb_InputSetEventProviders(NULL, NULL, NULL);
    jpb_InputSetControllerStateProvider(NULL, NULL);
    return 0;
}

static int test_direct_input_owners(void)
{
    JPBDirectInputDeviceVtbl vtable;
    TestDirectInputDevice source;
    TestDirectInputDevice found;
    JPB_GUID property = {1, 2, 3, {4, 5, 6, 7, 8, 9, 10, 11}};
    JPB_GUID zero_guid;
    SDL_GameController *const controller =
        (SDL_GameController *)(uintptr_t)0x7100u;
    int joystick_count = 2;

    memset(&vtable, 0, sizeof(vtable));
    vtable.queryInterface = direct_input_query;
    vtable.release = direct_input_release;
    vtable.setProperty = direct_input_set_property;
    vtable.acquire = direct_input_acquire;
    vtable.unacquire = direct_input_unacquire;
    memset(&source, 0, sizeof(source));
    memset(&found, 0, sizeof(found));
    source.device.lpVtbl = &vtable;
    found.device.lpVtbl = &vtable;
    source.queryResult = &found.device;
    memset(&zero_guid, 0, sizeof(zero_guid));
    CHECK(memcmp(&IID_IDirectInputDevice2A, &zero_guid, sizeof(zero_guid)) == 0);

    memset(g_rgpdevFound, 0, sizeof(g_rgpdevFound));
    g_cpdevFound = 0;
    AddInputDevice(&source.device, NULL);
    CHECK(g_cpdevFound == 1);
    CHECK(g_rgpdevFound[0] == &found.device);
    CHECK(source.queryCalls == 1);
    CHECK(source.lastIid == &IID_IDirectInputDevice2A);
    g_cpdevFound = 10;
    AddInputDevice(&source.device, NULL);
    CHECK(g_cpdevFound == 10);
    CHECK(source.queryCalls == 1);

    memset(g_rgpdevFound, 0, sizeof(g_rgpdevFound));
    g_pdevCurrent = &source.device;
    g_rgpdevFound[0] = &found.device;
    g_rgpdevFound[1] = &source.device;
    g_cpdevFound = 2;
    CleanupInput();
    CHECK(source.unacquireCalls == 1);
    CHECK(source.releaseCalls == 1);
    CHECK(found.releaseCalls == 1);
    CHECK(g_pdevCurrent == NULL);
    CHECK(g_rgpdevFound[0] == NULL);
    CHECK(g_rgpdevFound[1] == NULL);
    CHECK(g_cpdevFound == 0);

    CHECK(ReacquireInput() == 0);
    g_pdevCurrent = &source.device;
    source.acquireStatus = -1;
    CHECK(ReacquireInput() == 0);
    source.acquireStatus = 0;
    CHECK(ReacquireInput() == 1);
    CHECK(source.acquireCalls == 2);
    g_pdevCurrent = NULL;

    source.setPropertyStatus = -7;
    CHECK(SetDIDwordProperty(
              &source.device, &property, 0x1122, 0x3344, 0x5566) == -7);
    CHECK(source.setPropertyCalls == 1);
    CHECK(source.lastProperty == &property);
    CHECK(source.property.header.size == 20);
    CHECK(source.property.header.headerSize == 16);
    CHECK(source.property.header.object == 0x1122);
    CHECK(source.property.header.how == 0x3344);
    CHECK(source.property.value == 0x5566);

    CHECK(PickInputDevice(0) == 1);
    CHECK(ReadGameInput == ReadKeyboardInput);
    CHECK(PickInputDevice(-1) == 1);
    CHECK(ReadGameInput == (JPBInputReadGameFunction)ReadJoystickInput);

    gGameControllers[1] = NULL;
    CHECK(assignBackToP1() == 0);
    gGameControllers[0] = NULL;
    gGameControllers[1] = controller;
    p2Connected = 4;
    padExist = 0;
    lastUsedInputType = 0;
    firstRun = 9;
    g_pdevCurrent = &source.device;
    g_rgpdevFound[0] = &found.device;
    g_cpdevFound = 1;
    jpb_InputSetJoystickCountProvider(
        read_joystick_count, &joystick_count);
    CHECK(assignBackToP1() == 1);
    CHECK(gGameControllers[0] == controller);
    CHECK(gGameControllers[1] == NULL);
    CHECK(p2Connected == 0);
    CHECK(padExist == 1);
    CHECK(lastUsedInputType == 1);
    CHECK(firstRun == 9);
    CHECK(g_pdevCurrent == NULL);
    CHECK(g_cpdevFound == 0);
    CHECK(source.unacquireCalls == 2);
    CHECK(found.releaseCalls == 2);

    joystick_count = 0;
    gGameControllers[1] = controller;
    lastUsedInputType = 5;
    firstRun = 9;
    CHECK(assignBackToP1() == 1);
    CHECK(lastUsedInputType == 0);
    CHECK(firstRun == 0);

    debugOut(L"unused");
    g_pdevCurrent = NULL;
    memset(g_rgpdevFound, 0, sizeof(g_rgpdevFound));
    g_cpdevFound = 0;
    memset(gGameControllers, 0, sizeof(gGameControllers));
    ReadGameInput = ReadKeyboardInput;
    jpb_InputSetJoystickCountProvider(NULL, NULL);
    return 0;
}

static int test_input_initialization(void)
{
    int joystick_count = 3;

    CHECK(p1JoyIndex == -1);
    CHECK(p2PadIndex == -1);
    CHECK(firstRun == 1);
    CHECK(XBOX_MAP[6] == 4);
    CHECK(PS5_MAP[6] == 20);
    CHECK(SWITCH_PRO_MAP[0] == 1);
    CHECK(SWITCH_PRO_MAP[1] == 0);
    CHECK(controlLimits[0] == 16);
    CHECK(controlLimits[8] == 106);
    CHECK(JOYSTICK_DEAD_ZONE == 0.25f);
    CHECK(JOYSTICK_RUN_LIMIT == 0.98f);
    CHECK(ReadGameInput == ReadKeyboardInput);

    lastUsedInputType = 0;
    firstRun = 9;
    jpb_InputSetJoystickCountProvider(
        read_joystick_count, &joystick_count);
    CHECK(InitInput() == 3);
    CHECK(lastUsedInputType == 1);
    CHECK(firstRun == 9);

    joystick_count = 0;
    lastUsedInputType = 7;
    firstRun = 9;
    initInput();
    CHECK(lastUsedInputType == 0);
    CHECK(firstRun == 0);
    jpb_InputSetJoystickCountProvider(NULL, NULL);
    return 0;
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
    SDL_GameController *const p1_controller =
        (SDL_GameController *)(uintptr_t)0x1111u;
    SDL_GameController *const p2_controller =
        (SDL_GameController *)(uintptr_t)0x2222u;

    p1Disconnected = 7;
    p2Disconnected = 8;
    p2Connected = 9;
    gGameControllers[0] = p1_controller;
    gGameControllers[1] = p2_controller;
    P1Disconnected();
    CHECK(p1Disconnected == 7);
    CHECK(p2Disconnected == 8);
    CHECK(p2Connected == 9);
    CHECK(gGameControllers[0] == p1_controller);
    CHECK(gGameControllers[1] == p2_controller);

    P2Disconnected();
    CHECK(p1Disconnected == 7);
    CHECK(p2Disconnected == 1);
    CHECK(p2Connected == 9);
    CHECK(gGameControllers[0] == p1_controller);
    CHECK(gGameControllers[1] == NULL);

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

    vibration_stop(-1);
    CHECK(rumble.calls == 2);

    vibration_start(-1, (ShockEffect){"signed-bound", 1, 1, 1, 0});
    CHECK(rumble.calls == 3);
    CHECK(rumble.controller == -1);
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
    CHECK(test_input_initialization() == 0);
    CHECK(test_keyboard_owner() == 0);
    CHECK(test_joystick_owner() == 0);
    CHECK(test_controller_device_lifecycle() == 0);
    CHECK(test_start_device_owner() == 0);
    CHECK(test_player_two_join_owner() == 0);
    CHECK(test_direct_input_owners() == 0);
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
