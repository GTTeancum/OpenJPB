/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\Work\input.c.
 *
 * The reviewed subset owns the platform-neutral pad masks, held-button
 * suppression, rising-edge detection, and continuous-button behavior. The
 * reference SDL/DirectInput read is represented by a narrow provider callback
 * so PC and nxdk adapters can feed the same game-owned state machine.
 *
 * Provenance:
 *   direct     - names/signatures from the exact PDB; PADLOAD type 0x6AF7;
 *                pad globals from linked symbols.
 *   decompiled - control flow checked against the raw Ghidra export.
 *   assembly   - complete mask/update expression, bit-15 pad-index clearing,
 *                store ordering, zero ranges, and the initInput tail jump
 *                checked at exact RVAs.
 *
 * PDB module: 0043
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\input.obj
 * Primary source: W:\SWJediPowerBattles\Work\input.c
 * Compiler language: c
 * Emitted procedures: 11
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/input.h"
#include "jpb/game.h"

#include <string.h>

static JPBInputReadProvider input_provider;
static void *input_provider_user_data;
static JPBInputPowerBattleChordProvider
    power_battle_chord_provider;
static void *power_battle_chord_user_data;
static JPBInputKeyPressedProvider key_pressed_provider;
static void *key_pressed_user_data;

/* Exact linked packet/type globals at RVAs 0x4CBAD0 and 0x538050. */
ControllerPacket padBuffer[JPB_INPUT_PAD_COUNT];
uint8_t padTypes;

typedef struct Shocker {
    int32_t dataSize;
    int32_t value;
} Shocker;

/* Exact file-local PDB globals and initialized table. */
static char ShockBuffer[2][6];
static Shocker shocker[2][6];
static int32_t effectTimer[2];
static int32_t effectPriority[2];
static ShockEffect effects[15] = {
    {"none", 0, 0, 0, 5},
    {"hitsoft", 2, 0, 64, 0},
    {"hitmed", 4, 0, 64, 1},
    {"hithard", 4, 255, 64, 1},
    {"hitwall", 1, 0, 255, 0},
    {"hitperson", 2, 255, 128, 1},
    {"hitweapon", 3, 255, 128, 1},
    {"blockshot", 2, 255, 0, 2},
    {"fireblast", 2, 0, 160, 0},
    {"firemissl", 6, 255, 64, 1},
    {"splodnear", 4, 255, 180, 1},
    {"splodefar", 2, 255, 80, 0},
    {"pncperson", 3, 0, 160, 1},
    {"forcepush", 3, 0, 80, 0},
    {"uiselect", 1, 80, 80, 1},
};

void jpb_InputSetProvider(
    JPBInputReadProvider provider, void *user_data)
{
    input_provider = provider;
    input_provider_user_data = user_data;
}

uint32_t jpb_InputReadRawPad(int32_t pad_index)
{
    if (input_provider == NULL) {
        return 0;
    }
    return input_provider(pad_index, input_provider_user_data);
}

void jpb_InputSetPowerBattleChordProvider(
    JPBInputPowerBattleChordProvider provider,
    void *user_data)
{
    power_battle_chord_provider = provider;
    power_battle_chord_user_data = user_data;
}

int jpb_InputPowerBattleChordPressed(void)
{
    if (power_battle_chord_provider == NULL) {
        return 0;
    }
    return power_battle_chord_provider(
        power_battle_chord_user_data) != 0;
}

void jpb_InputSetKeyPressedProvider(
    JPBInputKeyPressedProvider provider,
    void *user_data)
{
    key_pressed_provider = provider;
    key_pressed_user_data = user_data;
}

int jpb_InputKeyPressed(int virtual_key)
{
    return key_pressed_provider != NULL &&
           key_pressed_provider(
               virtual_key, key_pressed_user_data) != 0;
}

/* 0xAEBD0, 45 bytes, global, 0 named locals
 * ClearInput
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void ClearInput(void)
{
    memset(padMaskBits, 0, sizeof(padMaskBits));
    memset(padCurrentBits, 0, sizeof(padCurrentBits));
}

/* 0xAEC00, 291 bytes, global, 6 named locals
 * PadGone
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
int PadGone(int p)
{
    int flag;
    uint8_t padbit;

    flag = padBuffer[p].transStatus != 0;
    if (padBuffer[p].dataFormat != (uint8_t)'A' &&
        padBuffer[p].dataFormat != (uint8_t)'s') {
        flag = 1;
    }
    padbit = (uint8_t)(1u << (unsigned)p);

    if ((padExist & padbit) != 0 && flag != 0) {
        padTypes &= (uint8_t)~padbit;
        if ((padShockable & padbit) != 0) {
            clearShockers(0, p);
        }
        padShockable &= (uint8_t)~padbit;
        padExist &= (uint8_t)~padbit;
        return flag;
    }

    if (flag != 0) {
        padExist &= (uint8_t)~padbit;
    } else {
        padExist |= padbit;
        if ((padTypes & padbit) == 0) {
            if (padBuffer[p].dataFormat == (uint8_t)'s') {
                padTypes |= padbit;
            }
        } else if (padBuffer[p].dataFormat == (uint8_t)'A') {
            padTypes &= (uint8_t)~padbit;
        }
    }
    return flag;
}

/* 0xAED30, 84 bytes, global, 2 named locals
 * clearShockers
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void clearShockers(int type, int padnum)
{
    int32_t shockerIndex;

    (void)type;
    effectTimer[padnum] = 0;
    effectPriority[padnum] = -1;
    for (shockerIndex = 0;
         shockerIndex < nShockers[padnum];
         ++shockerIndex) {
        shocker[padnum][shockerIndex].dataSize = 0;
    }
    vibration_stop(padnum);
}

/* 0xAED90, 182 bytes, global, 2 named locals
 * feedback_startEffect
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void feedback_startEffect(int padnum, int effect)
{
    if (OptionStruct.ShockFlag[padnum] != 0 &&
        padnum < 2 &&
        (unsigned int)effect < 15u) {
        ShockEffect selected = effects[effect];

        effectTimer[padnum] = selected.timer;
        effectPriority[padnum] = selected.priority;
        shocker[padnum][0].value = selected.power1;
        shocker[padnum][1].value = selected.power2;
        vibration_start(padnum, selected);
    }
}

/* 0xAEE50, 247 bytes, global, 2 named locals
 * handleShockers
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void handleShockers(int padnum)
{
    int32_t shockerIndex;
    char *shbuff;

    if (padnum < 2 && nShockers[padnum] > 0) {
        shbuff = ShockBuffer[padnum];
        for (shockerIndex = 0;
             shockerIndex < nShockers[padnum];
             ++shockerIndex) {
            if (shocker[padnum][shockerIndex].dataSize >= 0) {
                if (shocker[padnum][shockerIndex].dataSize == 0) {
                    shbuff[shockerIndex] =
                        shocker[padnum][shockerIndex].value != 0;
                } else {
                    shbuff[shockerIndex] = (char)
                        shocker[padnum][shockerIndex].value;
                }
            }
        }

        --effectTimer[padnum];
        if (effectTimer[padnum] <= 0) {
            effectTimer[padnum] = 0;
            effectPriority[padnum] = -1;
            for (shockerIndex = 0;
                 shockerIndex < nShockers[padnum];
                 ++shockerIndex) {
                shocker[padnum][shockerIndex].dataSize = 0;
            }
            vibration_stop(padnum);
        }
    }
}

/* 0xAEF50, 5 bytes, global, 0 named locals
 * initInput
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void initInput(void)
{
    (void)InitInput();
}

/* 0xAEF60, 16 bytes, global, 0 named locals
 * initPSXPad
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void initPSXPad(void)
{
    nShockers[0] = 0;
    nShockers[1] = 0;
    padExist = 0;
}

/* 0xAEF70, 189 bytes, global, 7 named locals
 * input_ReadControlPad
 * PDB type: unsigned long (long, unsigned, u...
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
uint32_t input_ReadControlPad(
    int32_t padnum,
    uint32_t continuousMask,
    uint32_t *oldBits)
{
    int32_t index = padnum & JPB_INPUT_PAD_INDEX_MASK;
    uint32_t input;
    uint32_t maskedBits;
    uint32_t previousBits;

    setMenuBindings(GameStruct.inMenuFlag, GameStruct.gameMode);
    input = jpb_InputReadRawPad(index);
    maskedBits = input & (~input | padMaskBits[index]);
    previousBits = *oldBits;

    padMaskBits[index] = ~input | padMaskBits[index];
    *oldBits = maskedBits;
    return ((maskedBits ^ previousBits) | continuousMask) & maskedBits;
}

/* 0xAF030, 195 bytes, global, 5 named locals
 * maskPadBits
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void maskPadBits(int padnum)
{
    int index = padnum & JPB_INPUT_PAD_INDEX_MASK;
    uint32_t input;
    uint32_t mask;

    padMaskBits[padnum] = 0;
    setMenuBindings(GameStruct.inMenuFlag, GameStruct.gameMode);
    input = jpb_InputReadRawPad(index);
    mask = ~input | padMaskBits[index];
    padMaskBits[index] = mask;
    padMaskBits[padnum] = mask & input;
}

/* 0xAF100, 25 bytes, global, 0 named locals
 * psxUpdatePadbits
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void psxUpdatePadbits(void)
{
    handleShockers(0);
    handleShockers(1);
}

/* 0xAF120, 121 bytes, global, 2 named locals
 * startRumble
 * PDB type: void (int, int)
 * Source: W:\SWJediPowerBattles\Work\input.c
 */
void startRumble(int controllerIndex, int duration)
{
    ShockEffect selected;

    (void)duration;
    nShockers[controllerIndex] = 1;
    if (OptionStruct.ShockFlag[0] != 0) {
        selected = effects[1];
        effectTimer[0] = selected.timer;
        effectPriority[0] = selected.priority;
        shocker[0][0].value = selected.power1;
        shocker[0][1].value = selected.power2;
        vibration_start(0, selected);
    }
}
