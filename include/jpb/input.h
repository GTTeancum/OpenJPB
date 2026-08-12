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
typedef void (*JPBInputRumbleProvider)(
    int32_t controller_index,
    uint16_t low_frequency,
    uint16_t high_frequency,
    uint32_t duration_ms,
    void *user_data);

/*
 * Exact PDB type name used by the two input-mode globals. The original enum
 * members come from the SDL-facing Windows layer; gameplay only distinguishes
 * zero from nonzero, so the portable core retains the fixed-width ABI without
 * importing SDL headers.
 */
typedef int32_t SDL_InputType;

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
extern int32_t p2Connected;
extern int32_t p1Disconnected;
extern int32_t p2Disconnected;
extern int32_t inMenuState;
extern int32_t inTitleState;
extern float g_p1X;
extern float g_p1Y;
extern float g_p2X;
extern float g_p2Y;

void jpb_InputSetProvider(
    JPBInputReadProvider provider, void *user_data);
uint32_t jpb_InputReadRawPad(int32_t pad_index);
void jpb_InputSetPowerBattleChordProvider(
    JPBInputPowerBattleChordProvider provider,
    void *user_data);
int jpb_InputPowerBattleChordPressed(void);
void jpb_InputSetRumbleProvider(
    JPBInputRumbleProvider provider,
    void *user_data);
void feedback_startEffect(int padnum, int effect);
void vibration_start(
    int controllerIndex, ShockEffect effect);
void vibration_stop(int controllerIndex);

void ClearInput(void);
int PadGone(int p);
void clearShockers(int type, int padnum);
void handleShockers(int padnum);
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

#undef JPB_INPUT_STATIC_ASSERT

#endif
