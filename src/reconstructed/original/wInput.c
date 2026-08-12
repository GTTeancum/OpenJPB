#include "jpb/input.h"

/*
 * GENERATED RECONSTRUCTION SHELL - no function bodies recovered here.
 * PDB module: 0103
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\wInput.obj
 * Primary source: W:\SWJediPowerBattles\work\wInput.c
 * Compiler language: c
 * Emitted procedures: 26
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

static JPBInputRumbleProvider rumble_provider;
static void *rumble_provider_user_data;

/* Exact linked wInput global at matched-PC RVA 0x92DACC. */
int32_t p2Connected;
/* Exact menu-binding globals at matched-PC RVAs 0x92DAD0/0x92DAD4. */
int32_t inMenuState;
int32_t inTitleState;

void jpb_InputSetRumbleProvider(
    JPBInputRumbleProvider provider,
    void *user_data)
{
    rumble_provider = provider;
    rumble_provider_user_data = user_data;
}

/* 0x12BE30, 571 bytes, global, 3 named locals
 * ReadKeyboardInput
 * PDB type: unsigned long ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C070, 356 bytes, global, 3 named locals
 * AddControllerDevice
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C1E0, 45 bytes, global, 3 named locals
 * AddInputDevice
 * PDB type: void (IDirectInputDeviceA*, cons...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C210, 3 bytes, global, 1 named locals
 * AddJoyDevice
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C220, 119 bytes, global, 3 named locals
 * CleanupInput
 * PDB type: void ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C2A0, 3 bytes, global, 4 named locals
 * DirectDrawCreateEx
 * PDB type: HRESULT (_GUID*, void**, const _...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C2B0, 3 bytes, global, 3 named locals
 * DirectDrawEnumerateExA
 * PDB type: HRESULT (HRESULT (_GUID*, char*,...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

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

/* 0x12C320, 211 bytes, global, 5 named locals
 * InitKeyboardInput
 * PDB type: int (IDirectInputA*)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C400, 3 bytes, global, 0 named locals
 * P1Disconnected
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void P1Disconnected(void)
{
    /* Exact optimized body: P1 always retains the keyboard fallback. */
}

/* 0x12C410, 22 bytes, global, 0 named locals
 * P2Disconnected
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */
void P2Disconnected(void)
{
    p2Disconnected = 1;
    p2Connected = 0;
}

/* 0x12C430, 33 bytes, global, 1 named locals
 * PickInputDevice
 * PDB type: int (int)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C460, 43 bytes, global, 2 named locals
 * ReacquireInput
 * PDB type: int ()
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12C490, 2476 bytes, global, 24 named locals
 * ReadJoystickInput
 * PDB type: unsigned long (int, int, int, in...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12CE40, 82 bytes, global, 6 named locals
 * SetDIDwordProperty
 * PDB type: HRESULT (IDirectInputDeviceA*, c...
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12CEA0, 145 bytes, global, 1 named locals
 * UpdateJoyDevices
 * PDB type: void (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

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

/* 0x12D040, 592 bytes, global, 7 named locals
 * checkP2Device
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12D290, 371 bytes, global, 4 named locals
 * checkStartDevice
 * PDB type: int (<no type>)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

/* 0x12D410, 3 bytes, global, 1 named locals
 * debugOut
 * PDB type: void (const wchar_t*)
 * Source: W:\SWJediPowerBattles\work\wInput.c
 */

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
    if (controllerIndex < 3 &&
        rumble_provider != NULL) {
        rumble_provider(
            controllerIndex,
            0,
            0,
            0,
            rumble_provider_user_data);
    }
}
