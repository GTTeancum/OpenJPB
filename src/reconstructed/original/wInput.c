#include "jpb/input.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * COMPLETE REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\wInput.c.
 *
 * The recovered module owns keyboard/controller translation, controller
 * lifecycle and join selection, rumble dispatch, and the dormant DirectInput
 * compatibility block. Undefined returns and the uninitialized keyboard
 * device local are retained because direct instructions prove them.
 *
 * Provenance:
 *   direct     - function names, signatures, locals, types, and global layouts
 *                from the exact PDB.
 *   decompiled - all 26 control-flow bodies checked against the raw export.
 *   assembly   - initialized data, branch asymmetries, hot-plug selection,
 *                COM vtable offsets, no-op exports, and undefined dormant
 *                behavior checked at exact RVAs in the shipped executable.
 *
 * PDB module: 0103
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wInput.obj
 * Primary source: W:\SWJediPowerBattles\work\wInput.c
 * Compiler language: c
 * Emitted procedures: 26
 */

static JPBInputRumbleProvider rumble_provider;
static void *rumble_provider_user_data;
static JPBInputJoystickCountProvider joystick_count_provider;
static void *joystick_count_provider_user_data;
static JPBInputKeyboardStateProvider keyboard_state_provider;
static void *keyboard_state_provider_user_data;
static JPBInputControllerStateProvider controller_state_provider;
static void *controller_state_provider_user_data;
static JPBInputControllerOpenProvider controller_open_provider;
static JPBInputControllerAttachedProvider controller_attached_provider;
static void *controller_device_provider_user_data;
static JPBInputEventPollProvider event_poll_provider;
static JPBInputControllerLookupProvider controller_lookup_provider;
static void *event_provider_user_data;

/* Exact initialized SDL button maps at RVAs 0x4D4960/0x4D49B0/0x4D4A10. */
int32_t XBOX_MAP[17] = {
    0, 1, 2, 3, 9, 10, 4, 6, 4, 5, 7, 8, 11, 12, 14, 13, 5
};
int32_t PS5_MAP[17] = {
    0, 1, 2, 3, 9, 10, 20, 6, 4, 5, 7, 8, 11, 12, 14, 13, 5
};
int32_t SWITCH_PRO_MAP[17] = {
    1, 0, 3, 2, 9, 10, 4, 6, 4, 5, 7, 8, 11, 12, 14, 13, 5
};
float JOYSTICK_DEAD_ZONE = 0.25f;
float JOYSTICK_RUN_LIMIT = 0.98f;
int32_t p1JoyIndex = -1;
int32_t p2PadIndex = -1;
/* Exact BSS input override at matched-PC RVA 0x92DA88. */
int32_t mini2_keyboardOverride;

/* Exact linked SDL controller globals at matched-PC RVAs 0x92DA90/0x92DAA0. */
SDL_GameController *gGameControllers[2];
SDL_GameController *sdlPads[5];
/* Exact initialized threshold table at matched-PC RVA 0x4D4A58. */
uint8_t controlLimits[9] = {
    0x10, 0x1a, 0x24, 0x2e, 0x38, 0x42, 0x4c, 0x56, 0x6a
};
/* Exact linked wInput global at matched-PC RVA 0x92DACC. */
int32_t p2Connected;
/* Exact menu-binding globals at matched-PC RVAs 0x92DAD0/0x92DAD4. */
int32_t inMenuState;
int32_t inTitleState;
/* Exact initialized function pointer at matched-PC RVA 0x4D4A08. */
JPBInputReadGameFunction ReadGameInput = ReadKeyboardInput;

/* Exact DirectInput BSS block at matched-PC RVA 0x932C00. */
JPB_GUID IID_IDirectInputDevice2A = {0};
JPBDirectInputDevice *g_pdevCurrent;
JPBDirectInputDevice *g_rgpdevFound[10];
int32_t g_cpdevFound;

void jpb_InputSetRumbleProvider(
    JPBInputRumbleProvider provider,
    void *user_data)
{
    rumble_provider = provider;
    rumble_provider_user_data = user_data;
}

void jpb_InputSetJoystickCountProvider(
    JPBInputJoystickCountProvider provider,
    void *user_data)
{
    joystick_count_provider = provider;
    joystick_count_provider_user_data = user_data;
}

void jpb_InputSetKeyboardStateProvider(
    JPBInputKeyboardStateProvider provider,
    void *user_data)
{
    keyboard_state_provider = provider;
    keyboard_state_provider_user_data = user_data;
}

void jpb_InputSetControllerStateProvider(
    JPBInputControllerStateProvider provider,
    void *user_data)
{
    controller_state_provider = provider;
    controller_state_provider_user_data = user_data;
}

void jpb_InputSetControllerDeviceProviders(
    JPBInputControllerOpenProvider open_provider,
    JPBInputControllerAttachedProvider attached_provider,
    void *user_data)
{
    controller_open_provider = open_provider;
    controller_attached_provider = attached_provider;
    controller_device_provider_user_data = user_data;
}

void jpb_InputSetEventProviders(
    JPBInputEventPollProvider poll_provider,
    JPBInputControllerLookupProvider lookup_provider,
    void *user_data)
{
    event_poll_provider = poll_provider;
    controller_lookup_provider = lookup_provider;
    event_provider_user_data = user_data;
}

/* 0x12BE30, 571 bytes, global, 3 named locals
 * ReadKeyboardInput
 * PDB type: unsigned long ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
uint32_t jpb_InputMapKeyboardState(
    const JPBKeyboardState *keyboard,
    int in_menu_state,
    int in_title_state,
    float *axis_x,
    float *axis_y)
{
    uint32_t bits = 0;
    float x = 0.0f;
    float y = 0.0f;

    if (keyboard == NULL) {
        return 0;
    }
    if (keyboard->moveUp) {
        bits = JPB_PAD_UP;
        y = -1.0f;
    }
    if (keyboard->moveLeft) {
        bits |= JPB_PAD_RIGHT;
        x = -1.0f;
    }
    if (keyboard->moveDown) {
        bits |= JPB_PAD_DOWN;
        y += 1.0f;
    }
    if (keyboard->moveRight) {
        bits |= JPB_PAD_LEFT;
        x += 1.0f;
    }
    if (keyboard->walkModifier &&
        (keyboard->moveUp || keyboard->moveLeft ||
         keyboard->moveDown || keyboard->moveRight)) {
        bits |= JPB_PAD_ANALOG_MOVEMENT;
    }
    if (keyboard->zoomIn) bits |= JPB_PAD_ZOOM_IN;
    if (keyboard->comboWest) bits |= JPB_PAD_COMBO_WEST;
    if (keyboard->comboNorth) bits |= JPB_PAD_COMBO_NORTH;
    if (keyboard->lockOn) bits |= JPB_PAD_LOCK_ON;
    if (keyboard->jumpBlockChord) bits |= UINT32_C(0x25);
    if (keyboard->northBlockChord) bits |= UINT32_C(0x15);
    if (keyboard->southBlockChord) bits |= UINT32_C(0x45);
    if (keyboard->westBlockChord) bits |= UINT32_C(0x85);

    if (in_menu_state == 0) {
        if (in_title_state == 0 &&
            (keyboard->space || keyboard->enter)) {
            bits |= JPB_PAD_ZOOM_IN;
        }
        if (keyboard->block) bits |= UINT32_C(0x5);
        if (keyboard->escape) bits |= JPB_PAD_START;
        if (keyboard->comboSouth) bits |= JPB_PAD_COMBO_SOUTH;
        if (keyboard->space || keyboard->enter) bits |= JPB_PAD_JUMP;
        if (mini2_keyboardOverride != 0 && keyboard->space) {
            bits |= JPB_PAD_COMBO_NORTH;
        }
    } else {
        if (keyboard->block) bits |= JPB_PAD_BLOCK;
        if (keyboard->escape) bits |= JPB_PAD_JUMP;
        if (keyboard->space || keyboard->enter) {
            bits |= in_title_state == 0
                ? JPB_PAD_START
                : JPB_PAD_COMBO_SOUTH;
        }
    }
    if (axis_x != NULL) *axis_x = x;
    if (axis_y != NULL) *axis_y = y;
    return bits;
}

uint32_t ReadKeyboardInput(void)
{
    const JPBKeyboardState *keyboard = keyboard_state_provider != NULL
        ? keyboard_state_provider(keyboard_state_provider_user_data)
        : NULL;

    if (keyboard == NULL) {
        return 0;
    }
    return jpb_InputMapKeyboardState(
        keyboard, inMenuState, inTitleState, &g_p1X, &g_p1Y);
}

/* 0x12C070, 356 bytes, global, 3 named locals
 * AddControllerDevice
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void AddControllerDevice(int device)
{
    SDL_GameController *controller;
    int index;
    int scan;

    if (p2Disconnected == 0) {
        for (index = 0; index < 5 && sdlPads[index] != NULL; ++index) {
        }
        if (index == 5) {
            return;
        }
        controller = controller_open_provider != NULL
            ? controller_open_provider(
                  device, controller_device_provider_user_data)
            : NULL;
        sdlPads[index] = controller;
        if (gGameControllers[0] == NULL) {
            gGameControllers[0] = controller;
        }
        return;
    }

    p2Disconnected = 0;
    controller = controller_open_provider != NULL
        ? controller_open_provider(device, controller_device_provider_user_data)
        : NULL;
    gGameControllers[1] = controller;
    index = p2PadIndex;
    sdlPads[index] = controller;
    if (gGameControllers[0] == controller) {
        gGameControllers[0] = NULL;
        lastUsedInputType = 0;
        for (scan = 0; scan < 5; ++scan) {
            if (scan != index && sdlPads[scan] != NULL) {
                gGameControllers[0] = sdlPads[scan];
                lastUsedInputType = 1;
            }
        }
    }
}

/* 0x12C1E0, 45 bytes, global, 3 named locals
 * AddInputDevice
 * PDB type: void (IDirectInputDeviceA*, cons...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void AddInputDevice(
    JPBDirectInputDevice *device,
    const JPBDirectInputDeviceInstance *instance)
{
    int index = g_cpdevFound;

    (void)instance;
    if (index < 10) {
        ++g_cpdevFound;
        device->lpVtbl->queryInterface(
            device,
            &IID_IDirectInputDevice2A,
            (void **)&g_rgpdevFound[index]);
    }
}

/* 0x12C210, 3 bytes, global, 1 named locals
 * AddJoyDevice
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void AddJoyDevice(int device)
{
    (void)device;
}

/* 0x12C220, 119 bytes, global, 3 named locals
 * CleanupInput
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void CleanupInput(void)
{
    int index;

    if (g_pdevCurrent != NULL) {
        g_pdevCurrent->lpVtbl->unacquire(g_pdevCurrent);
        g_pdevCurrent = NULL;
    }
    for (index = 0; index < g_cpdevFound; ++index) {
        if (g_rgpdevFound[index] != NULL) {
            g_rgpdevFound[index]->lpVtbl->release(g_rgpdevFound[index]);
            g_rgpdevFound[index] = NULL;
        }
    }
    g_cpdevFound = 0;
}

/* 0x12C2A0, 3 bytes, global, 4 named locals
 * DirectDrawCreateEx
 * PDB type: HRESULT (_GUID*, void**, const _...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
JPBHRESULT DirectDrawCreateEx(
    JPB_GUID *guid,
    void **direct_draw,
    const JPB_GUID *iid,
    void *outer_unknown)
{
    (void)guid;
    (void)direct_draw;
    (void)iid;
    (void)outer_unknown;
}

/* 0x12C2B0, 3 bytes, global, 3 named locals
 * DirectDrawEnumerateExA
 * PDB type: HRESULT (HRESULT (_GUID*, char*,...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
JPBHRESULT DirectDrawEnumerateExA(
    JPBDirectDrawEnumerateCallback callback,
    void *context,
    uint32_t flags)
{
    (void)callback;
    (void)context;
    (void)flags;
}

/* 0x12C2C0, 71 bytes, global, 0 named locals
 * InitInput
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C310, 3 bytes, global, 2 named locals
 * InitJoystickInput
 * PDB type: int (const DIDEVICEINSTANCEA*, v...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int InitJoystickInput(
    const JPBDirectInputDeviceInstance *instance,
    void *reference)
{
    (void)instance;
    (void)reference;
}

/* 0x12C320, 211 bytes, global, 5 named locals
 * InitKeyboardInput
 * PDB type: int (IDirectInputA*)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int InitKeyboardInput(void *direct_input)
{
    JPBDirectInputDevice *device;
    JPBDIPropertyDword property;
    JPBDirectInputDeviceInstance instance;
    JPBHRESULT result;

    (void)direct_input;
    property.header.size = sizeof(property);
    property.header.headerSize = sizeof(property.header);
    property.header.object = 0;
    property.header.how = 0;
    property.value = 32;
    result = device->lpVtbl->setProperty(
        device,
        (const JPB_GUID *)(uintptr_t)1,
        &property.header);
    if (result == 0) {
        instance.size = sizeof(instance);
        result = device->lpVtbl->getDeviceInfo(device, &instance);
        if (result == 0) {
            AddInputDevice(device, &instance);
            device->lpVtbl->release(device);
            return 1;
        }
    }
    device->lpVtbl->release(device);
    return 0;
}

/* 0x12C400, 3 bytes, global, 0 named locals
 * P1Disconnected
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int InitInput(void)
{
    int joystick_count = joystick_count_provider != NULL
        ? joystick_count_provider(joystick_count_provider_user_data)
        : 0;

    if (joystick_count < 1) {
        lastUsedInputType = 0;
        printf("Warning: No joysticks connected!\n");
        firstRun = 0;
        return 1;
    }
    lastUsedInputType = 1;
    return joystick_count;
}
void P1Disconnected(void)
{
    /* Exact optimized body: P1 always retains the keyboard input path. */
}

/* 0x12C410, 22 bytes, global, 0 named locals
 * P2Disconnected
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void P2Disconnected(void)
{
    p2Disconnected = 1;
    gGameControllers[1] = NULL;
}

/* 0x12C430, 33 bytes, global, 1 named locals
 * PickInputDevice
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int PickInputDevice(int device)
{
    ReadGameInput = device == 0
        ? (JPBInputReadGameFunction)ReadKeyboardInput
        : (JPBInputReadGameFunction)ReadJoystickInput;
    return 1;
}

/* 0x12C460, 43 bytes, global, 2 named locals
 * ReacquireInput
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int ReacquireInput(void)
{
    if (g_pdevCurrent != NULL &&
        g_pdevCurrent->lpVtbl->acquire(g_pdevCurrent) >= 0) {
        return 1;
    }
    return 0;
}

/* 0x12C490, 2476 bytes, global, 24 named locals
 * ReadJoystickInput
 * PDB type: unsigned long (int, int, int, in...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
static const int32_t *input_controller_map(const char *name)
{
    if (name != NULL &&
        (strcmp(name, "PS5 Controller") == 0 ||
         strcmp(name, "PS4 Controller") == 0)) {
        return PS5_MAP;
    }
    if (name != NULL &&
        (strcmp(name, "Nintendo Switch Pro Controller") == 0 ||
         strcmp(name, "Nintendo Switch Joy-Con (L/R)") == 0)) {
        return SWITCH_PRO_MAP;
    }
    return XBOX_MAP;
}

static float input_controller_limit(uint8_t limit_index)
{
    float limit = 127.0f;

    if ((float)controlLimits[limit_index] <= 127.0f) {
        limit = (float)controlLimits[limit_index];
    }
    return limit / 127.0f;
}

uint32_t jpb_InputMapControllerState(
    const JPBControllerState *controller,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu_state,
    float *axis_x,
    float *axis_y)
{
    const int32_t *map;
    uint32_t bits = 0;
    float x;
    float y;
    float walk_threshold;
    float run_threshold;

    if (axis_x != NULL) *axis_x = 0.0f;
    if (axis_y != NULL) *axis_y = 0.0f;
    if (controller == NULL) {
        return 0;
    }
    x = (float)controller->axis[0] / 32767.0f;
    y = (float)controller->axis[1] / 32767.0f;
    if (axis_x != NULL) *axis_x = x;
    if (axis_y != NULL) *axis_y = y;
    if (controller->name == NULL) {
        return 0;
    }

    map = input_controller_map(controller->name);
    walk_threshold = input_controller_limit(walk_limit);
    run_threshold = input_controller_limit(run_limit);
    JOYSTICK_DEAD_ZONE = walk_threshold;
    JOYSTICK_RUN_LIMIT = run_threshold;

    if (x < -walk_threshold) bits |= JPB_PAD_RIGHT;
    else if (x > walk_threshold) bits |= JPB_PAD_LEFT;
    if (y < -walk_threshold) bits |= JPB_PAD_UP;
    else if (y > walk_threshold) bits |= JPB_PAD_DOWN;
    if (bits == 0) {
        x = 0.0f;
        y = 0.0f;
    }
    if (sqrtf(((float)controller->axis[0] / 32767.0f) *
              ((float)controller->axis[0] / 32767.0f) +
              ((float)controller->axis[1] / 32767.0f) *
              ((float)controller->axis[1] / 32767.0f)) > run_threshold) {
        bits |= JPB_PAD_ANALOG_MOVEMENT;
    }

    if ((controller->hat & 1u) != 0 || controller->button[map[12]]) {
        bits |= JPB_PAD_UP;
        y -= 1.0f;
    }
    if ((controller->hat & 4u) != 0 || controller->button[map[13]]) {
        bits |= JPB_PAD_DOWN;
        y += 1.0f;
    }
    if ((controller->hat & 8u) != 0 || controller->button[map[15]]) {
        bits |= JPB_PAD_RIGHT;
        x -= 1.0f;
    }
    if ((controller->hat & 2u) != 0 || controller->button[map[14]]) {
        bits |= JPB_PAD_LEFT;
        x += 1.0f;
    }

    if (controller_config == 0 || in_menu_state != 0) {
        if (controller->button[map[0]]) bits |= JPB_PAD_COMBO_SOUTH;
        if (controller->button[map[1]]) bits |= JPB_PAD_JUMP;
        if (controller->button[map[2]]) bits |= JPB_PAD_COMBO_WEST;
        if (controller->button[map[3]]) bits |= JPB_PAD_COMBO_NORTH;
    } else {
        if (controller->button[map[0]]) bits |= JPB_PAD_JUMP;
        if (controller->button[map[1]]) bits |= JPB_PAD_COMBO_NORTH;
        if (controller->button[map[2]]) bits |= JPB_PAD_COMBO_WEST;
        if (controller->button[map[3]]) bits |= JPB_PAD_COMBO_SOUTH;
    }
    if (controller->button[map[4]]) bits |= JPB_PAD_BLOCK;
    if (controller->button[map[5]]) bits |= JPB_PAD_LOCK_ON;
    if (controller->button[map[6]]) bits |= JPB_PAD_ZOOM_IN;
    if (controller->button[map[7]]) bits |= JPB_PAD_START;
    if (controller->axis[4] > 0) bits |= JPB_PAD_LEFT_TRIGGER;
    if (controller->axis[5] > 0) bits |= JPB_PAD_RIGHT_TRIGGER;

    if (axis_x != NULL) *axis_x = x;
    if (axis_y != NULL) *axis_y = y;
    return bits;
}

static uint32_t input_read_controller(
    SDL_GameController *controller,
    int walk_limit,
    int run_limit,
    int controller_config,
    float *axis_x,
    float *axis_y)
{
    JPBControllerState state;

    memset(&state, 0, sizeof(state));
    if (controller_state_provider == NULL ||
        !controller_state_provider(
            controller, &state, controller_state_provider_user_data)) {
        return 0;
    }
    return jpb_InputMapControllerState(
        &state,
        (uint8_t)walk_limit,
        (uint8_t)run_limit,
        (uint8_t)controller_config,
        inMenuState,
        axis_x,
        axis_y);
}

uint32_t ReadJoystickInput(
    int padnum,
    int walk_limit,
    int run_limit,
    int controller_config)
{
    uint32_t bits;
    int index;

    if (padnum == 0) {
        bits = ReadKeyboardInput();
        if (bits != 0) {
            lastUsedInputType = 0;
            gGameControllers[0] = NULL;
            player1InputType = 0;
            return bits;
        }
        if (joystick_count_provider == NULL ||
            joystick_count_provider(joystick_count_provider_user_data) < 1) {
            player1InputType = 0;
            return 0;
        }
    }
    if (padnum == 1) {
        return input_read_controller(
            gGameControllers[1],
            walk_limit,
            run_limit,
            controller_config,
            &g_p2X,
            &g_p2Y);
    }

    for (index = 0; index < 5; ++index) {
        if (sdlPads[index] != NULL &&
            sdlPads[index] != gGameControllers[1]) {
            bits = input_read_controller(
                sdlPads[index],
                walk_limit,
                run_limit,
                controller_config,
                &g_p1X,
                &g_p1Y);
            if (bits != 0) {
                player1InputType = 1;
                lastUsedInputType = 1;
                gGameControllers[padnum] = sdlPads[index];
                return bits;
            }
        }
    }
    return 0;
}

/* 0x12CE40, 82 bytes, global, 6 named locals
 * SetDIDwordProperty
 * PDB type: HRESULT (IDirectInputDeviceA*, c...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
JPBHRESULT SetDIDwordProperty(
    JPBDirectInputDevice *device,
    const JPB_GUID *property,
    uint32_t object,
    uint32_t how,
    uint32_t value)
{
    JPBDIPropertyDword data;

    data.header.size = sizeof(data);
    data.header.headerSize = sizeof(data.header);
    data.header.object = object;
    data.header.how = how;
    data.value = value;
    return device->lpVtbl->setProperty(device, property, &data.header);
}

/* 0x12CEA0, 145 bytes, global, 1 named locals
 * UpdateJoyDevices
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void UpdateJoyDevices(void)
{
    int index;

    if (controller_attached_provider == NULL) {
        return;
    }
    for (index = 0; index < 5; ++index) {
        if (sdlPads[index] != NULL &&
            controller_attached_provider(
                sdlPads[index], controller_device_provider_user_data) == 0) {
            sdlPads[index] = NULL;
            if (index == p2PadIndex) {
                p2Disconnected = 1;
                gGameControllers[1] = NULL;
            }
            return;
        }
    }
}

/* 0x12CF40, 12 bytes, global, 0 named locals
 * WInput_IsKBM
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12CF50, 240 bytes, global, 4 named locals
 * assignBackToP1
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int assignBackToP1(void)
{
    if (gGameControllers[1] == NULL) {
        return 0;
    }
    gGameControllers[0] = gGameControllers[1];
    gGameControllers[1] = NULL;
    p2Connected = 0;
    padExist = 1;
    CleanupInput();
    (void)InitInput();
    return 1;
}

/* 0x12D040, 592 bytes, global, 7 named locals
 * checkP2Device
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int checkP2Device(void)
{
    JPBInputEvent event;

    while (event_poll_provider != NULL &&
           event_poll_provider(&event, event_provider_user_data) != 0) {
        if (event.type == UINT32_C(0x651)) {
            SDL_GameController *controller;
            JPBControllerState state;
            int is_switch;
            int join_button;
            int cancel_button;
            int index;

            if (controller_lookup_provider == NULL) {
                continue;
            }
            controller = controller_lookup_provider(
                event.controllerInstanceId, event_provider_user_data);
            memset(&state, 0, sizeof(state));
            if (controller_state_provider == NULL ||
                !controller_state_provider(
                    controller,
                    &state,
                    controller_state_provider_user_data) ||
                state.name == NULL) {
                continue;
            }
            is_switch =
                strcmp(state.name, "Nintendo Switch Pro Controller") == 0 ||
                strcmp(state.name, "Nintendo Switch Joy-Con (L/R)") == 0;
            join_button = is_switch
                ? event.controllerButton == 1
                : event.controllerButton == 0;
            cancel_button = is_switch
                ? event.controllerButton == 0
                : event.controllerButton == 1;
            if (join_button) {
                controller = controller_lookup_provider(
                    event.controllerInstanceId, event_provider_user_data);
                for (index = 0; index < 5; ++index) {
                    int scan;

                    if (sdlPads[index] != controller) {
                        continue;
                    }
                    if (g_isSteamDeck != 0 &&
                        controller == gGameControllers[0]) {
                        return 0;
                    }
                    padExist |= 2u;
                    if (controller == gGameControllers[0]) {
                        gGameControllers[0] = NULL;
                        lastUsedInputType = 0;
                        for (scan = 0; scan < 5; ++scan) {
                            if (scan != index && sdlPads[scan] != NULL) {
                                gGameControllers[0] = sdlPads[scan];
                                lastUsedInputType = 1;
                            }
                        }
                    }
                    p2PadIndex = index;
                    gGameControllers[1] = controller;
                    p2Connected = 1;
                    return 1;
                }
            } else if (cancel_button) {
                return -1;
            }
        } else if (event.type == UINT32_C(0x300) &&
                   event.keycode == 0x1b) {
            return -1;
        }
    }
    p2Connected = 0;
    gGameControllers[1] = NULL;
    p2PadIndex = -1;
    return 0;
}

/* 0x12D290, 371 bytes, global, 4 named locals
 * checkStartDevice
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int checkStartDevice(void)
{
    int index;

    for (index = 0; index < 5; ++index) {
        if (sdlPads[index] != NULL) {
            JPBControllerState controller;

            memset(&controller, 0, sizeof(controller));
            if (controller_state_provider == NULL ||
                !controller_state_provider(
                    sdlPads[index],
                    &controller,
                    controller_state_provider_user_data) ||
                controller.name == NULL) {
                continue;
            }
            if (controller.button[input_controller_map(controller.name)[0]]) {
                ReadGameInput = ReadJoystickInput;
                padExist = 1;
                gGameControllers[0] = sdlPads[index];
                return 1;
            }
        }
        if (keyboard_state_provider != NULL) {
            const JPBKeyboardState *keyboard = keyboard_state_provider(
                keyboard_state_provider_user_data);

            if (keyboard != NULL && (keyboard->enter || keyboard->space)) {
                lastUsedInputType = 0;
                ReadGameInput = ReadKeyboardInput;
                gGameControllers[0] = NULL;
                gGameControllers[1] = NULL;
                return 1;
            }
        }
    }
    return 0;
}

/* 0x12D410, 3 bytes, global, 1 named locals
 * debugOut
 * PDB type: void (const wchar_t*)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void debugOut(const wchar_t *output)
{
    (void)output;
}

/* 0x12D420, 26 bytes, global, 2 named locals
 * padbuttonpressed
 * PDB type: int (int, int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
int WInput_IsKBM(void)
{
    return lastUsedInputType == 0;
}
int padbuttonpressed(int pad, int button)
{
    return (padCurrentBits[pad].padLevel1 &
            (UINT32_C(1) << (unsigned)button)) != 0;
}

/* 0x12D440, 13 bytes, global, 2 named locals
 * setMenuBindings
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void setMenuBindings(int state, int gameState)
{
    inMenuState = state;
    inTitleState = gameState;
}

/* 0x12D450, 106 bytes, global, 5 named locals
 * vibration_start
 * PDB type: void (int, ShockEffect)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void vibration_start(
    int controllerIndex, ShockEffect effect)
{
    if (controllerIndex < 3 &&
        rumble_provider != NULL) {
        uint16_t low_frequency =
            (uint16_t)((double)effect.power1 *
                       65535.0 / 255.0);
        uint16_t high_frequency =
            (uint16_t)((double)effect.power2 *
                       65535.0 / 255.0);

        rumble_provider(
            controllerIndex,
            low_frequency,
            high_frequency,
            (uint32_t)(effect.timer * 100),
            rumble_provider_user_data);
    }
}

/* 0x12D4C0, 59 bytes, global, 1 named locals
 * vibration_stop
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void vibration_stop(int controllerIndex)
{
    if ((unsigned)controllerIndex < 3u &&
        rumble_provider != NULL) {
        rumble_provider(
            controllerIndex,
            0,
            0,
            0,
            rumble_provider_user_data);
    }
}
