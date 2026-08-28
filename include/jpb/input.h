#ifndef JPB_INPUT_H
#define JPB_INPUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JPB_INPUT_PAD_COUNT = 4,
    JPB_INPUT_PAD_INDEX_MASK = ~0x8000,
    /* Exact bits emitted by the matched PC ReadJoystickInput routine. */
    JPB_PAD_LEFT_TRIGGER = 0x0001,
    JPB_PAD_RIGHT_TRIGGER = 0x0002,
    JPB_PAD_BLOCK = 0x0004,
    JPB_PAD_LOCK_ON = 0x0008,
    JPB_PAD_COMBO_NORTH = 0x0010,
    JPB_PAD_FORCE_POWER_1 = 0x0010,
    JPB_PAD_JUMP = 0x0020,
    JPB_PAD_COMBO_SOUTH = 0x0040,
    JPB_PAD_FORCE_POWER_3 = 0x0040,
    JPB_PAD_COMBO_WEST = 0x0080,
    JPB_PAD_ITEM = 0x0080,
    JPB_PAD_BUTTON_1 = JPB_PAD_FORCE_POWER_3,
    JPB_PAD_ZOOM_IN = 0x0100,
    JPB_PAD_ZOOM_OUT = 0x0200,
    /*
     * Exact movement qualifier consumed through gaButtonMap[][0]. A stick
     * above RunLimit emits it; the keyboard's left-Control walk modifier also
     * emits it. brain_ControlPlayer interprets it together with input type.
     */
    JPB_PAD_ANALOG_MOVEMENT = 0x0400,
    JPB_PAD_START = 0x0800,
    JPB_PAD_UP = 0x1000,
    JPB_PAD_LEFT = 0x2000,
    JPB_PAD_DOWN = 0x4000,
    JPB_PAD_RIGHT = 0x8000
};

/* Direct PDB type 0x6AF7. */
typedef struct PADLOAD {
    uint32_t padLevel1;
    uint32_t padLevel2;
} PADLOAD;

/* Direct PDB type 0x6B19. The packet payload is a 32-byte anonymous union;
 * pad is its first recovered member and raw preserves the complete extent. */
typedef union ControllerPacketData {
    uint16_t pad;
    uint8_t raw[32];
} ControllerPacketData;

typedef struct ControllerPacket {
    uint8_t transStatus;
    uint8_t dataFormat;
    ControllerPacketData data;
} ControllerPacket;

/* Exact matched-PC PDB type 0x6AF1. */
typedef struct ShockEffect {
    char *name;
    int32_t timer;
    int32_t power1;
    int32_t power2;
    int32_t priority;
} ShockEffect;

typedef uint32_t (*JPBInputReadProvider)(
    int32_t pad_index, void *user_data);
typedef int (*JPBInputPowerBattleChordProvider)(void *user_data);
typedef int (*JPBInputKeyPressedProvider)(
    int virtual_key, void *user_data);
typedef void (*JPBInputRumbleProvider)(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data);
typedef int (*JPBInputJoystickCountProvider)(void *user_data);

/*
 * Exact PDB type name used by the two input-mode globals. The original enum
 * members come from the SDL-facing Windows layer; gameplay only distinguishes
 * zero from nonzero, so the portable core retains the fixed-width ABI without
 * importing SDL headers.
 */
typedef int32_t SDL_InputType;

/* Exact opaque SDL owner used by the matched PC input globals. */
typedef struct _SDL_GameController SDL_GameController;

/* Physical key state consumed by the recovered ReadKeyboardInput owner. */
typedef struct JPBKeyboardState {
    uint8_t moveUp;
    uint8_t moveLeft;
    uint8_t moveDown;
    uint8_t moveRight;
    uint8_t walkModifier;
    uint8_t zoomIn;
    uint8_t comboSouth;
    uint8_t comboWest;
    uint8_t comboNorth;
    uint8_t lockOn;
    uint8_t jumpBlockChord;
    uint8_t northBlockChord;
    uint8_t southBlockChord;
    uint8_t westBlockChord;
    uint8_t block;
    uint8_t space;
    uint8_t enter;
    uint8_t escape;
} JPBKeyboardState;

typedef const JPBKeyboardState *(*JPBInputKeyboardStateProvider)(
    void *user_data);

typedef struct JPBControllerState {
    const char *name;
    int32_t axis[6];
    uint8_t button[21];
    uint8_t hat;
} JPBControllerState;

typedef int (*JPBInputControllerStateProvider)(
    SDL_GameController *controller,
    JPBControllerState *state,
    void *user_data);
typedef SDL_GameController *(*JPBInputControllerOpenProvider)(
    int device,
    void *user_data);
typedef int (*JPBInputControllerAttachedProvider)(
    SDL_GameController *controller,
    void *user_data);
typedef uint32_t (*JPBInputReadGameFunction)();

typedef struct JPBInputEvent {
    uint32_t type;
    int32_t controllerInstanceId;
    uint8_t controllerButton;
    int32_t keycode;
} JPBInputEvent;

typedef int (*JPBInputEventPollProvider)(
    JPBInputEvent *event,
    void *user_data);
typedef SDL_GameController *(*JPBInputControllerLookupProvider)(
    int32_t instance_id,
    void *user_data);

typedef int32_t JPBHRESULT;

typedef struct JPB_GUID {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} JPB_GUID;

typedef struct JPBDIPropertyHeader {
    uint32_t size;
    uint32_t headerSize;
    uint32_t object;
    uint32_t how;
} JPBDIPropertyHeader;

typedef struct JPBDIPropertyDword {
    JPBDIPropertyHeader header;
    uint32_t value;
} JPBDIPropertyDword;

typedef struct JPBDirectInputDeviceInstance {
    uint32_t size;
    uint8_t data[0x240];
} JPBDirectInputDeviceInstance;

typedef struct JPBDirectInputDevice JPBDirectInputDevice;

typedef struct JPBDirectInputDeviceVtbl {
    JPBHRESULT (*queryInterface)(
        JPBDirectInputDevice *device,
        const JPB_GUID *iid,
        void **result);
    uint32_t (*addRef)(JPBDirectInputDevice *device);
    uint32_t (*release)(JPBDirectInputDevice *device);
    void *getCapabilities;
    void *enumObjects;
    void *getProperty;
    JPBHRESULT (*setProperty)(
        JPBDirectInputDevice *device,
        const JPB_GUID *property,
        const JPBDIPropertyHeader *header);
    JPBHRESULT (*acquire)(JPBDirectInputDevice *device);
    JPBHRESULT (*unacquire)(JPBDirectInputDevice *device);
    void *getDeviceState;
    void *getDeviceData;
    void *setDataFormat;
    void *setEventNotification;
    void *setCooperativeLevel;
    void *getObjectInfo;
    JPBHRESULT (*getDeviceInfo)(
        JPBDirectInputDevice *device,
        JPBDirectInputDeviceInstance *instance);
} JPBDirectInputDeviceVtbl;

struct JPBDirectInputDevice {
    const JPBDirectInputDeviceVtbl *lpVtbl;
};

typedef JPBHRESULT (*JPBDirectDrawEnumerateCallback)(
    JPB_GUID *guid,
    char *description,
    char *name,
    void *context,
    void *monitor);

extern uint32_t padMaskBits[JPB_INPUT_PAD_COUNT];
extern PADLOAD padCurrentBits[JPB_INPUT_PAD_COUNT];
extern ControllerPacket padBuffer[JPB_INPUT_PAD_COUNT];
extern uint8_t padTypes;
extern uint8_t padExist;
extern uint8_t padShockable;
extern int32_t nShockers[2];
extern SDL_InputType player1InputType;
extern SDL_InputType player2InputType;
extern int32_t lastUsedInputType;
extern int32_t firstRun;
extern int32_t p2Connected;
extern int32_t p1Disconnected;
extern int32_t p2Disconnected;
extern int32_t inMenuState;
extern int32_t inTitleState;
extern SDL_GameController *gGameControllers[2];
extern SDL_GameController *sdlPads[5];
extern int32_t XBOX_MAP[17];
extern int32_t PS5_MAP[17];
extern int32_t SWITCH_PRO_MAP[17];
extern uint8_t controlLimits[9];
extern float JOYSTICK_DEAD_ZONE;
extern float JOYSTICK_RUN_LIMIT;
extern int32_t p1JoyIndex;
extern int32_t p2PadIndex;
extern int32_t mini2_keyboardOverride;
extern float g_p1X;
extern float g_p1Y;
extern float g_p2X;
extern float g_p2Y;
extern JPBInputReadGameFunction ReadGameInput;
extern int32_t g_isSteamDeck;
extern JPB_GUID IID_IDirectInputDevice2A;
extern JPBDirectInputDevice *g_pdevCurrent;
extern JPBDirectInputDevice *g_rgpdevFound[10];
extern int32_t g_cpdevFound;

void jpb_InputSetProvider(
    JPBInputReadProvider provider, void *user_data);
uint32_t jpb_InputReadRawPad(int32_t pad_index);
void jpb_InputSetPowerBattleChordProvider(
    JPBInputPowerBattleChordProvider provider,
    void *user_data);
int jpb_InputPowerBattleChordPressed(void);
void jpb_InputSetKeyPressedProvider(
    JPBInputKeyPressedProvider provider,
    void *user_data);
int jpb_InputKeyPressed(int virtual_key);
void jpb_InputSetRumbleProvider(
    JPBInputRumbleProvider provider,
    void *user_data);
void jpb_InputSetJoystickCountProvider(
    JPBInputJoystickCountProvider provider,
    void *user_data);
void jpb_InputSetKeyboardStateProvider(
    JPBInputKeyboardStateProvider provider,
    void *user_data);
void jpb_InputSetControllerStateProvider(
    JPBInputControllerStateProvider provider,
    void *user_data);
void jpb_InputSetControllerDeviceProviders(
    JPBInputControllerOpenProvider open_provider,
    JPBInputControllerAttachedProvider attached_provider,
    void *user_data);
void jpb_InputSetEventProviders(
    JPBInputEventPollProvider poll_provider,
    JPBInputControllerLookupProvider lookup_provider,
    void *user_data);
uint32_t jpb_InputMapKeyboardState(
    const JPBKeyboardState *keyboard,
    int in_menu_state,
    int in_title_state,
    float *axis_x,
    float *axis_y);
uint32_t ReadKeyboardInput(void);
uint32_t jpb_InputMapControllerState(
    const JPBControllerState *controller,
    uint8_t walk_limit,
    uint8_t run_limit,
    uint8_t controller_config,
    int in_menu_state,
    float *axis_x,
    float *axis_y);
uint32_t ReadJoystickInput(
    int padnum,
    int walk_limit,
    int run_limit,
    int controller_config);
void AddControllerDevice(int device);
void AddJoyDevice(int device);
void UpdateJoyDevices(void);
int checkP2Device(void);
int checkStartDevice(void);
void AddInputDevice(
    JPBDirectInputDevice *device,
    const JPBDirectInputDeviceInstance *instance);
void CleanupInput(void);
#if !defined(__DDRAW_INCLUDED__)
JPBHRESULT DirectDrawCreateEx(
    JPB_GUID *guid,
    void **direct_draw,
    const JPB_GUID *iid,
    void *outer_unknown);
JPBHRESULT DirectDrawEnumerateExA(
    JPBDirectDrawEnumerateCallback callback,
    void *context,
    uint32_t flags);
#endif
int InitJoystickInput(
    const JPBDirectInputDeviceInstance *instance,
    void *reference);
int InitKeyboardInput(void *direct_input);
int PickInputDevice(int device);
int ReacquireInput(void);
JPBHRESULT SetDIDwordProperty(
    JPBDirectInputDevice *device,
    const JPB_GUID *property,
    uint32_t object,
    uint32_t how,
    uint32_t value);
int assignBackToP1(void);
void debugOut(const wchar_t *output);
void feedback_startEffect(int padnum, int effect);
void vibration_start(
    int controllerIndex, ShockEffect effect);
void vibration_stop(int controllerIndex);

void ClearInput(void);
int PadGone(int p);
void clearShockers(int type, int padnum);
void handleShockers(int padnum);
void initInput(void);
void initPSXPad(void);
uint32_t input_ReadControlPad(
    int32_t padnum,
    uint32_t continuousMask,
    uint32_t *oldBits);
void maskPadBits(int padnum);
void psxUpdatePadbits(void);
void startRumble(int controllerIndex, int duration);
void P1Disconnected(void);
void P2Disconnected(void);
int InitInput(void);
int WInput_IsKBM(void);
int padbuttonpressed(int pad, int button);
void setMenuBindings(int state, int gameState);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
#define JPB_INPUT_STATIC_ASSERT(condition, message) \
    static_assert(condition, message)
#else
#define JPB_INPUT_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)
#endif

JPB_INPUT_STATIC_ASSERT(sizeof(PADLOAD) == 8, "PADLOAD layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(ControllerPacketData) == 32,
    "ControllerPacket payload layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(ControllerPacket) == 34,
    "ControllerPacket must match PDB type 0x6B19");
JPB_INPUT_STATIC_ASSERT(
    sizeof(padBuffer) == 136,
    "padBuffer must match PDB array type 0x6AC9");
JPB_INPUT_STATIC_ASSERT(
    offsetof(ControllerPacket, dataFormat) == 1,
    "ControllerPacket.dataFormat layout changed");
JPB_INPUT_STATIC_ASSERT(
    offsetof(ControllerPacket, data) == 2,
    "ControllerPacket.data layout changed");
#if UINTPTR_MAX == UINT64_MAX
JPB_INPUT_STATIC_ASSERT(
    sizeof(ShockEffect) == 24,
    "ShockEffect must match PDB type 0x6AF1");
JPB_INPUT_STATIC_ASSERT(
    offsetof(ShockEffect, timer) == 8,
    "ShockEffect.timer layout changed");
#endif
JPB_INPUT_STATIC_ASSERT(
    offsetof(PADLOAD, padLevel1) == 0, "PADLOAD.padLevel1 layout changed");
JPB_INPUT_STATIC_ASSERT(
    offsetof(PADLOAD, padLevel2) == 4, "PADLOAD.padLevel2 layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(padMaskBits) == 16, "padMaskBits layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(padCurrentBits) == 32, "padCurrentBits layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(gGameControllers) == 16,
    "gGameControllers must match PDB type 0x7782");
JPB_INPUT_STATIC_ASSERT(
    sizeof(sdlPads) == 40,
    "sdlPads must match PDB type 0x75F1");
JPB_INPUT_STATIC_ASSERT(
    sizeof(controlLimits) == 9,
    "controlLimits must match PDB type 0x6AD7");
JPB_INPUT_STATIC_ASSERT(
    sizeof(XBOX_MAP) == 68,
    "XBOX_MAP must match PDB type 0x102E");
JPB_INPUT_STATIC_ASSERT(
    sizeof(PS5_MAP) == 68,
    "PS5_MAP must match PDB type 0x102E");
JPB_INPUT_STATIC_ASSERT(
    sizeof(SWITCH_PRO_MAP) == 68,
    "SWITCH_PRO_MAP must match PDB type 0x102E");
JPB_INPUT_STATIC_ASSERT(
    sizeof(JPBKeyboardState) == 18,
    "JPBKeyboardState platform boundary changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(JPB_GUID) == 16,
    "DirectInput GUID layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(JPBDIPropertyHeader) == 16,
    "DIPROPHEADER layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(JPBDIPropertyDword) == 20,
    "DIPROPDWORD layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(JPBDirectInputDeviceInstance) == 0x244,
    "DIDEVICEINSTANCEA layout changed");
JPB_INPUT_STATIC_ASSERT(
    sizeof(g_rgpdevFound) == 80,
    "g_rgpdevFound must match PDB type 0x11FBE");

#undef JPB_INPUT_STATIC_ASSERT

#endif
