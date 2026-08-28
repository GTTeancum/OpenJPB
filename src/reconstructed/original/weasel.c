/*
 * COMPLETE REVIEWED RECONSTRUCTION.
 *
 * All 11 emitted procedures were checked against matched PDB symbols and
 * types plus direct disassembly/decompilation of the shipped executable at
 * RVAs 0x108BD0..0x10ADA2. Static editor state, key-read order, authored
 * bounds, node labels, animation dispatch, and retail edge cases are kept.
 * PDB source: W:\SWJediPowerBattles\work\weasel.c
 */
#include "jpb/weasel.h"

#include "jpb/animctrl.h"
#include "jpb/animutil.h"
#include "jpb/brainutl.h"
#include "jpb/camera.h"
#include "jpb/collision.h"
#include "jpb/combo.h"
#include "jpb/console.h"
#include "jpb/debugtext.h"
#include "jpb/effects.h"
#include "jpb/ferret.h"
#include "jpb/game.h"
#include "jpb/input.h"
#include "jpb/physics.h"
#include "jpb/scene.h"
#include "jpb/sound.h"
#include "jpb/text.h"
#include "jpb/whook.h"

#include <string.h>

int32_t EDITNODE[2];
Motion *paMotions;
uint32_t lFrame;
uint32_t fFrame;

static int32_t idleData[4] = {0, 20, 21, 19};
static const uint32_t weaselMotionFlag[5] = {
    0x04000000U, 0x08000000U, 0x10000000U, 0x40000000U, 0x80000000U};
static const char *const weaselNodeName[18] = {
    "PELVIS_ID", "RT_UPPERLEG_ID", "RT_LOWERLEG_ID", "RT_FOOT_ID",
    "LF_UPPERLEG_ID", "LF_LOWERLEG_ID", "LF_FOOT_ID", "TORSO_ID",
    "HEAD_ID", "RT_UPPERARM_ID", "RT_LOWERARM_ID", "RT_HAND_ID",
    "LF_UPPERARM_ID", "LF_LOWERARM_ID", "LF_HAND_ID", "WEAPON_ID",
    "WEAPON_COLLISION_1_ID", "WEAPON_COLLISION_2_ID"};

static uint32_t gOldBits;
static uint32_t gOldBits2;
static int MODE;
static int seq = 0x41;
static int rate;
static int idletype;
static int bank;
static uint8_t alpha;
static int height;
static int comboID;
static int comboEdit;
static int lastlen = -1;
static int lastseq = -1;
static int mechanicsEdit = 1;
static int combatEdit = 3;
static int effectsEdit = 2;
static int flagsEdit = 1;
static int basicEdit = 3;

static int weasel_EditDelta(void)
{
    int delta = KeyPressed(0x1e) != 0;
    if (KeyPressed(0x1f)) {
        delta = -1;
    }
    return delta;
}

static void weasel_DrawSpinner(int count)
{
    int index;

    for (index = 0; index < count; ++index) {
        debug_printf("    \t");
    }
    debug_printf("  %s\n", spinName[spin++]);
    if (spin < 0) {
        spin = 5;
    } else if (spin > 5) {
        spin = 0;
    }
}

static int weasel_Clamp(int value, int minimum, int maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

/* PDB 0x108BD0; shipped body size 115 bytes. */
void weasel_DumpFlags(uint32_t flags)
{
    if (flags & 0x04000000U) debug_printf("HOLD ");
    if (flags & 0x08000000U) debug_printf(" KO  ");
    if (flags & 0x10000000U) debug_printf("NTWEEN ");
    if (flags & 0x40000000U) debug_printf("AIR ");
    if (flags & 0x80000000U) debug_printf("LOOP ");
    debug_printf("\t%x\n", flags);
}

/* PDB 0x109370; shipped body size 932 bytes. */
void weasel_EditBasic(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int delta;
    int value;

    (void)cpad;
    (void)player;
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-BASIC EDITING-");
    debug_printf("\n\n\n");
    weasel_DumpFlags(motion->motionFlags);
    debug_printf(" SPD\t ACC\t CHG\t ACC\tTWNI\tTWNO\tFRZI\tFRZO\n");
    debug_printf("%4d\t%4d\t%4d\t %3d\t%4d\t%4d\t%4d\t%4d\n",
        motion->Speed, motion->SpeedAcc, motion->Charge, motion->ChargeAcc,
        motion->twin, motion->twout, motion->frzin, motion->frzout);
    debug_printf("\n");
    weasel_DrawSpinner(basicEdit - 1);

    if (KeyPressed(0x1c)) --basicEdit;
    if (KeyPressed(0x1d)) ++basicEdit;
    brainutil_limitRange(&basicEdit, 1, 8);
    delta = weasel_EditDelta();

    switch (basicEdit) {
    case 1:
        motion->Speed = (int16_t)weasel_Clamp(
            motion->Speed + delta * 0x80, -1, 0x4000);
        break;
    case 2:
        motion->SpeedAcc = (int16_t)weasel_Clamp(
            motion->SpeedAcc + delta * 0x10, 0, 0x400);
        break;
    case 3:
        motion->Charge = (int16_t)weasel_Clamp(
            motion->Charge + delta * 4, -0x200, 0x200);
        break;
    case 4:
        value = motion->ChargeAcc + delta;
        if (value < -0x40) motion->ChargeAcc = -0x40;
        else motion->ChargeAcc =
            value > 0x80 ? -0x80 : (int16_t)(int8_t)value;
        break;
    case 5:
        motion->twin = (uint8_t)weasel_Clamp(motion->twin + delta, 0, 0x20);
        break;
    case 6:
        motion->twout = (uint8_t)weasel_Clamp(motion->twout + delta, 0, 0x20);
        break;
    case 7:
        motion->frzin = (int8_t)weasel_Clamp(
            motion->frzin + delta, 0, (int)(lFrame - fFrame));
        break;
    case 8:
        motion->frzout = (int8_t)weasel_Clamp(
            motion->frzout + delta, 0, (int)(lFrame - fFrame));
        break;
    }
    if (motion->reface == 0 && motion->Charge != 0) {
        debug_printf("\n\tNOTE: THIS MOVE WILL FACE TARGET WHEN STARTED\n");
    }
}

/* PDB 0x109720; shipped body size 884 bytes. */
void weasel_EditCombat(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int delta;
    int value;

    (void)cpad;
    (void)player;
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-COMBAT EDITING-");
    debug_printf("\n\n\n");
    weasel_DumpFlags(motion->motionFlags);
    debug_printf("DMG\tDLY\tRCT\tFTL\t RCL\tACC\n");
    debug_printf("%3d\t%3d\t%3d\t%3d\t%4d\t%4d\n",
        motion->Damage, motion->Delay, motion->hitReact, motion->combo,
        motion->Recoil, motion->RecoilAcc);
    weasel_DrawSpinner(combatEdit);

    if (KeyPressed(0x1c)) --combatEdit;
    if (KeyPressed(0x1d)) ++combatEdit;
    brainutil_limitRange(&combatEdit, 0, 5);
    delta = weasel_EditDelta();
    switch (combatEdit) {
    case 0:
        value = weasel_Clamp((int)motion->Damage + delta, -0x7f, 0x7f);
        motion->Damage = (uint8_t)value;
        break;
    case 1:
        motion->Delay = (uint8_t)weasel_Clamp(
            (int)motion->Delay + delta, 0, 0x40);
        break;
    case 2:
        value = weasel_Clamp((int)motion->hitReact + delta, -1, 0x41);
        if (value >= 0) debug_printf("\t\t\t%s\n", paMotions[value].name);
        else debug_printf("\t\t\tNORMAL REACTION\n");
        motion->hitReact = (uint8_t)value;
        break;
    case 3:
        value = weasel_Clamp((int)motion->combo + delta, -1, 0x41);
        if (value >= 0) debug_printf("\t\t\t%s\n", paMotions[value].name);
        else debug_printf("\t\t\tNORMAL REACTION\n");
        motion->combo = (uint8_t)value;
        break;
    case 4:
        motion->Recoil = (int16_t)weasel_Clamp(
            motion->Recoil + delta * 4, -0x200, 0x200);
        break;
    case 5:
        motion->RecoilAcc = (int16_t)weasel_Clamp(
            motion->RecoilAcc + delta, 0, 0x7ff);
        break;
    }
}

/* PDB 0x109AA0; shipped body size 1,462 bytes. */
int weasel_EditCombo(uint32_t *cpad, playerObject *player)
{
    uint32_t colors[5] = {
        0x7fffffffU, 0x7fffffffU, 0x7fffffffU, 0x7fffffffU, 0x7fffffffU};
    Combo *combo;
    int delta;
    int value;

    (void)cpad;
    _DrawText(160.0f, 160.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-COMBO MARMET(tm)-");
    colors[comboEdit] = 0x7fff80ffU;
    if (KeyPressed(0x1c)) --comboEdit;
    if (KeyPressed(0x1d)) ++comboEdit;
    if (comboEdit < 0) comboEdit = 4;
    else if (comboEdit > 4) comboEdit = 0;
    if (KeyPressed(9)) {
        if (ShiftKeyDown() && KeyPressed(9)) --comboID;
        else if (KeyPressed(9)) ++comboID;
        if (comboID < 0) comboID = 0x17;
        else if (comboID > 0x17) comboID = 0;
    }

    delta = weasel_EditDelta();
    combo = &player->paCombos[comboID];
    if (comboEdit == 0 && delta != 0) combo->comboFlags ^= 0x04000000U;
    if (comboEdit == 1) combo->Index = (uint8_t)(combo->Index + delta);
    if (player->maxMotions < (int16_t)(uint16_t)combo->Index) {
        combo->Index = (uint8_t)player->maxMotions;
    }
    if (comboEdit == 2) {
        value = combo->kdmin + delta;
        combo->kdmin = (int16_t)weasel_Clamp(value, 0, 0x18);
    } else if (comboEdit == 3) {
        value = combo->kdmax + delta;
        combo->kdmax = (int16_t)weasel_Clamp(value, 0, 0x18);
    } else if (comboEdit == 4) {
        value = combo->slack + delta;
        combo->slack = (int16_t)weasel_Clamp(value, 0, 0x18);
    }

    _DrawText(160.0f, 160.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "ID   = %d", comboID);
    _DrawText(36.0f, 184.0f, 0.0001f, 0.5f, colors[0],
        (combo->comboFlags & 0x04000000U) ? "alt  bank" : "norm bank");
    _DrawText(112.0f, 160.0f, 0.0001f, 0.7f, 0x7fffffffU,
        "COMBO = %s", combo->String);
    _DrawText(36.0f, 208.0f, 0.0001f, 0.5f, colors[1],
        "MOVE %2d %s(%d)", combo->Index, player->paMotions[combo->Index].name,
        player->paMotions[combo->Index].Lock);
    _DrawText(336.0f, 208.0f, 0.0001f, 0.5f, colors[2],
        "kdmin %d", combo->kdmin);
    _DrawText(408.0f, 208.0f, 0.0001f, 0.5f, colors[3],
        "kdmax %d", combo->kdmax);
    _DrawText(516.0f, 208.0f, 0.0001f, 0.5f, colors[4],
        "slk %d", combo->slack);
    if (!KeyPressed(10)) return 0;
    return (combo->comboFlags & 0x04000000U)
        ? -(int)combo->Index : (int)combo->Index;
}

/* PDB 0x10A060; shipped body size 956 bytes. */
void weasel_EditEffects(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int firstBank = 0;
    int secondBank = 0;
    int delta;
    int value;

    (void)cpad;
    (void)player;
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-EFFECTS EDITING-");
    debug_printf("\n\n\n\n");
    debug_printf("FX1\tFXD\tFX2\tFXD\tSD1\tSDD\tSD2\tSDD\n");
    debug_printf("%3d\t%3d\t%3d\t%3d\t%3d\t%3d\t%3d\t%3d\n",
        motion->fx1, motion->fx1Delay, motion->fx2, motion->fx2Delay,
        sound_GetSoundIndex(&firstBank, (char *)motion->snd[0]),
        motion->sndDelay[0],
        sound_GetSoundIndex(&secondBank, (char *)motion->snd[1]),
        motion->sndDelay[1]);
    weasel_DrawSpinner(effectsEdit);
    if (KeyPressed(0x1c)) --effectsEdit;
    if (KeyPressed(0x1d)) ++effectsEdit;
    brainutil_limitRange(&effectsEdit, 0, 5);
    delta = weasel_EditDelta();
    switch (effectsEdit) {
    case 0:
        motion->fx1 = (int8_t)weasel_Clamp(motion->fx1 + delta, -1, gMaxEffect);
        break;
    case 1:
        motion->fx1Delay = (int8_t)weasel_Clamp(
            motion->fx1Delay + delta, 0, (int)(lFrame - fFrame));
        break;
    case 2:
        motion->fx2 = (int8_t)weasel_Clamp(motion->fx2 + delta, -1, gMaxEffect);
        break;
    case 3:
        motion->fx2Delay = (int8_t)weasel_Clamp(
            motion->fx2Delay + delta, 0, (int)(lFrame - fFrame));
        break;
    case 4:
        value = sound_GetSoundIndex(&firstBank, (char *)motion->snd[0]) + delta;
        if (value >= -1) {
            if (value > sound_NumInBank(0)) value = sound_NumInBank(0);
            if (value >= 0) {
                strcpy((char *)motion->snd[0], sound_GetSoundName(0, value));
                debug_printf("\n\t\t\t\t%s\n", sound_GetSoundName(0, value));
                return;
            }
        }
        motion->snd[0][0] = '0';
        motion->snd[0][1] = '\0';
        debug_printf("\n\t\t\t\tNO SOUND\n");
        debug_printf("ADDED A ZERO!!\n");
        break;
    case 5:
        motion->sndDelay[0] = (int8_t)weasel_Clamp(
            motion->sndDelay[0] + delta, 0, 0x3c);
        break;
    }
}

/* PDB 0x10A420; shipped body size 500 bytes. */
void weasel_EditFlags(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int up;

    (void)cpad;
    (void)player;
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-FLAG EDITING-");
    debug_printf("\n\n\n");
    weasel_DumpFlags(motion->motionFlags);
    debug_printf("HLD\t KO\tNTW\tAIR\tLOP\n");
    debug_printf("%3d\t%3d\t%3d\t%3d\t%3d\n",
        (motion->motionFlags >> 26) & 1, (motion->motionFlags >> 27) & 1,
        (motion->motionFlags >> 28) & 1, (motion->motionFlags >> 30) & 1,
        (motion->motionFlags >> 31) & 1);
    weasel_DrawSpinner(flagsEdit);
    if (KeyPressed(0x1c)) --flagsEdit;
    if (KeyPressed(0x1d)) ++flagsEdit;
    brainutil_limitRange(&flagsEdit, 0, 3);
    up = KeyPressed(0x1e);
    if (KeyPressed(0x1f)) {
        motion->motionFlags &= ~weaselMotionFlag[flagsEdit];
    } else if (up && !(motion->motionFlags & weaselMotionFlag[flagsEdit])) {
        motion->motionFlags |= weaselMotionFlag[flagsEdit];
    }
}

/* PDB 0x10A620; shipped body size 820 bytes. */
void weasel_EditFrames(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int length = (int)(lFrame - fFrame);
    int firstDelta = 0;
    int lastDelta = 0;
    uint32_t firstColor = 0x7fffffffU;
    uint32_t lastColor = 0x7fffffffU;

    (void)cpad;
    (void)player;
    if (sequence != lastseq) {
        lastlen = length;
        lastseq = sequence;
    }
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-FRAME EDITING-");
    _DrawText(160.0f, 352.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "F = first frame L = last frame");
    if (KeyHeld('f')) {
        firstColor = 0x7fff8080U;
        firstDelta = -(KeyPressed(0x1c) != 0);
        if (KeyPressed(0x1d)) firstDelta = 1;
    } else if (KeyHeld('l')) {
        lastColor = 0x7fff8080U;
        lastDelta = -(KeyPressed(0x1c) != 0);
        if (KeyPressed(0x1d)) lastDelta = 1;
    }
    motion->cutin = (uint8_t)(motion->cutin + firstDelta);
    motion->cutout = (uint8_t)(motion->cutout + lastDelta);
    _DrawText(160.0f, 184.0f, 0.0001f, 0.5f, 0xff8080ffU,
        "LEN = %d CUR = %d", lastlen, length);
    _DrawText(444.0f, 184.0f, 0.0001f, 0.5f, firstColor, "%d", fFrame);
    _DrawText(516.0f, 184.0f, 0.0001f, 0.5f, lastColor, "%d", lFrame);
    _DrawText(444.0f, 208.0f, 0.0001f, 0.5f, 0xff8080ffU,
        "%d", motion->cutin);
    _DrawText(516.0f, 208.0f, 0.0001f, 0.5f, 0xff8080ffU,
        "%d", motion->cutout);
}

/* PDB 0x10A960; shipped body size 568 bytes. */
void weasel_EditMechanics(uint32_t *cpad, playerObject *player, int sequence)
{
    Motion *motion = &paMotions[sequence];
    int delta;

    (void)cpad;
    (void)player;
    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-MECHANICS EDITING-");
    debug_printf("\n\n\n");
    weasel_DumpFlags(motion->motionFlags);
    debug_printf("LCK\tDSP\t FAC\n");
    debug_printf("%3d\t%3d\t%4d\n", motion->Lock, motion->disp, motion->reface);
    weasel_DrawSpinner(mechanicsEdit);
    if (KeyPressed(0x1c)) --mechanicsEdit;
    if (KeyPressed(0x1d)) ++mechanicsEdit;
    brainutil_limitRange(&mechanicsEdit, 0, 2);
    delta = weasel_EditDelta();
    if (mechanicsEdit == 0) {
        motion->Lock = (uint8_t)weasel_Clamp(motion->Lock + delta, 0, 0xff);
    } else if (mechanicsEdit == 1) {
        motion->disp = (uint8_t)weasel_Clamp(
            motion->disp + delta, 0, (int)(lFrame - fFrame));
    } else {
        motion->reface = (int16_t)weasel_Clamp(
            motion->reface + delta * 0x200, -0x800, 0x800);
    }
    if ((uint16_t)(motion->reface + 1U) > 1U) {
        debug_printf("\n\tNOTE: THIS MOVE WILL TURN %d AT END\n", motion->reface);
    }
}

/* PDB 0x10ABA0; shipped body size 511 bytes. */
void weasel_EditNode(int32_t *cpad, playerObject *player)
{
    _svector rotation;
    int playernum = player->playernum;
    int node;

    _DrawText(160.0f, 112.0f, 0.0001f, 0.7f, 0xff8080ffU,
        "-NODE EDITING-");
    rotation.vx = (cpad[1] & 0x8000) ? -0x20 : 0;
    if (cpad[1] & 0x2000) rotation.vx = 0x20;
    rotation.vy = (cpad[1] & 0x1000) ? -0x20 : 0;
    if (cpad[1] & 0x4000) rotation.vy = 0x20;
    rotation.vz = (cpad[1] & 4) ? -0x20 : 0;
    if (cpad[1] & 8) rotation.vz = 0x20;
    rotation.pad = 0;
    if (cpad[0] & 0x40) player->pFlags &= ~4U;
    if (cpad[0] & 0x10) {
        _svector zero = {0, 0, 0, 0};
        coll_SetNodeRotationAbs(playernum, EDITNODE[playernum], &zero);
    }
    if (cpad[0] & 0x20) --EDITNODE[playernum];
    if (cpad[0] & 0x80) ++EDITNODE[playernum];
    if (EDITNODE[playernum] > 17) EDITNODE[playernum] = 0;
    if (EDITNODE[playernum] < 0) EDITNODE[playernum] = 17;
    coll_IncNodeRotationAbs(playernum, EDITNODE[playernum], &rotation);
    for (node = 0; node < 17; ++node) {
        coll_SetNodeFlags(playernum, node, 0x800000U);
    }
    debug_printf("\n\n\nNODE: %d %s\n", EDITNODE[playernum],
        weaselNodeName[EDITNODE[playernum]]);
    player->currentMotion = 0;
    animctrl_MotionNoLock(&player->playerRoot, paMotions);
}

/* PDB 0x108C50; shipped body size 1,816 bytes. */
void weasel_EditAnim(int32_t *cpad, playerObject *player)
{
    animObject *animation = (animObject *)
        ((sceneObject *)player->playerRoot.pParent)->pAnim;
    int max;
    int held;
    int selected;
    int framesMode = 0;

    ++alpha;
    if (bank == 0) {
        paMotions = player->paMotions;
        max = player->maxMotions;
    }
    anim_GetSeqFrameRange(
        &player->playerRoot, &paMotions[seq], (int *)&fFrame, (int *)&lFrame);
    cpad[0] = (int32_t)input_ReadControlPad(0, 0, &gOldBits);
    cpad[1] = (int32_t)input_ReadControlPad(0, 0xffffffffU, &gOldBits2);
    if (KeyPressed(0x1b)) {
        GameStruct.screenShotFlag = 0;
        game_gClrGameFlags(0x4000);
        camera_SetCurrentCameraType(1);
        sound_Play(NULL, 0, "good", 0);
        console_ReleaseKeyboard();
        return;
    }
    GameStruct.screenShotFlag = 1;
    if (KeyPressed(0x97)) { --MODE; height = 0; }
    if (KeyPressed(0x98)) { ++MODE; height = 0; }
    if (MODE < 0) MODE = 7;
    else if (MODE > 7) MODE = 0;
    debug_printf("\n\n\n\n");
    _DrawText(36.0f, 40.0f, 0.0001f, 0.6f, 0x7f80ff80U,
        "%d / 8\n", MODE + 1);
    if (height < 120) height += 10;
    menuBoxTest(16, 16, 592, (unsigned)height, 64, 64, 96);
    _DrawText(36.0f, 64.0f, 0.0001f, 0.6f,
        ((uint32_t)alpha << 24) | 0x00ffffffU,
        "WEASEL MOTION EDITOR(tm)\n");
    _DrawText(480.0f, 64.0f, 0.0001f, 0.6f, 0x7fffffffU,
        "Bank.Seq %d.%d\n", bank, seq);
    _DrawText(480.0f, 88.0f, 0.0001f, 0.6f, 0x7fffffffU,
        "%s", paMotions[seq].name);
    switch (MODE) {
    case 0: weasel_EditBasic((uint32_t *)cpad, player, seq); break;
    case 1: weasel_EditFlags((uint32_t *)cpad, player, seq); break;
    case 2: weasel_EditCombat((uint32_t *)cpad, player, seq); break;
    case 3: weasel_EditMechanics((uint32_t *)cpad, player, seq); break;
    case 4: weasel_EditEffects((uint32_t *)cpad, player, seq); break;
    case 5:
        weasel_EditFrames((uint32_t *)cpad, player, seq);
        framesMode = 1;
        break;
    case 6:
        selected = weasel_EditCombo((uint32_t *)cpad, player);
        if (selected == 0) return;
        seq = selected < 0 ? -selected : selected;
        MODE = 0;
        bank = (int)((uint32_t)selected >> 31);
        return;
    case 7: weasel_EditNode(cpad, player); break;
    }
    if (!framesMode) {
        if (animation->Lock > 22) {
            if (KeyPressed(0x20)) {
                animctrl_MotionNoLock(
                    &player->playerRoot, &paMotions[idleData[idletype]]);
            }
            debug_printf("PLAYING...\n");
            return;
        }
        if (KeyPressed('i')) ++idletype;
        if (idletype < 0) idletype = 3;
        else if (idletype > 3) idletype = 0;
        if (KeyPressed('b')) {
            bank ^= 1;
            sound_Play(NULL, 0, "good", 0);
        }
        held = KeyHeld(9);
        if (!held) rate = 0;
        if (KeyPressed(9) || KeyHeld(9)) {
            int initial = KeyPressed(9);
            held = KeyHeld(9);
            if (held) {
                rate += 0x200;
                if (rate > 0x1800) rate = 0x1800;
            }
            if (initial || rate >= 0x1000) {
                int step = (rate >> 12) + 1;
                if (ShiftKeyDown()) {
                    seq -= step;
                    if (seq < 0) seq = 0;
                } else {
                    seq += step;
                    if (seq > max - 1) seq = max - 1;
                }
                sound_Play(NULL, 0, "step1", 0);
            }
        }
    }
    if (cpad[1] & 0x2000) {
        physics_gForceFaceTarget(&player->playerRoot, &player->target->playerRoot);
        animctrl_MotionEqualLock(&player->playerRoot, &paMotions[26]);
    } else if (cpad[1] & 0x8000) {
        physics_gForceFaceTarget(&player->playerRoot, &player->target->playerRoot);
        animctrl_MotionEqualLock(&player->playerRoot, &paMotions[9]);
    } else if (!(cpad[0] & 0x800) && !KeyPressed(0x20)) {
        if (animctrl_MotionLockLevel(
                &player->playerRoot, &paMotions[idleData[idletype]], 0x16)) {
            physics_gClrConstantVector(&player->playerRoot);
        }
    } else {
        debug_printf("PLAYING %d\n", seq);
        animctrl_MotionComboChain(
            &player->playerRoot, &paMotions[seq], 0, 0, bank);
    }
}

/* PDB 0x10ADA0; shipped body is a three-byte return. */
void weasel_LevelBars(int current, int maximum, int color)
{
    (void)current;
    (void)maximum;
    (void)color;
}
