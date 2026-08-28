/*
 * Exact initialized title-menu definition streams recovered from matched
 * game.exe RVAs 0x4C5E70..0x4C6638. Names and extents come from game.pdb.
 * Values remain numeric because menu_mainMenu/mmDraw interpret this original
 * compact command format; they are not rewritten into an invented schema.
 */

#include "jpb/menu.h"
#include "jpb/game.h"
#include "jpb/world.h"

#define JPB_MOD(type_, inc_, min_, max_, src_, text_) \
    {(type_), (inc_), (min_), (max_), (src_), (text_), {0}}

/* Exact initialized PDB global at matched-PC RVA 0x4C1358. */
float menuTextDepthOverride = -1.0f;
/* Exact matched-PC globals at RVAs 0x4C6024 and 0x4C6028. */
float minSlider = -432.0f;
float maxSlider = 364.0f;

/* Exact PDB global at matched-PC RVA 0x4C8D00. */
char *snames[5] = {"obi", "qui", "mace", "adi", "plo"};
uint16_t playerSelectPix[23] = {
    220, 212, 163, 227, 221, 218, 49, 48,
    50, 51, 52, 53, 54, 55, 56, 57,
    58, 59, 60, 47, 223, 228, 217
};

/* Exact PDB globals at matched-PC RVAs 0x582640 and 0x582E40. */
RESOLUTION g_resolutions[256];
int32_t g_resolutionsCount;

/* Exact PDB mover streams at matched-PC RVAs 0x4C82C8..0x4C8327. */
uint8_t frameTopMover[24] = {
    0x32, 0x30, 0x34, 0x00, 0x24, 0x09, 0x00, 0xc7,
    0xff, 0x33, 0x25, 0x09, 0x00, 0x03, 0x00, 0x0a,
    0x39, 0x02, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x00
};
uint8_t frameBottomMover[24] = {
    0x32, 0x30, 0x34, 0x01, 0x2b, 0x14, 0x24, 0x1c,
    0x00, 0xd9, 0x00, 0x33, 0x25, 0x1c, 0x00, 0x9d,
    0x00, 0x0a, 0x39, 0x06, 0x37, 0x81, 0x2d, 0x00
};
uint8_t frameLeftMover[24] = {
    0x32, 0x30, 0x34, 0x03, 0x2b, 0x05, 0x24, 0x67,
    0xff, 0x0a, 0x00, 0x33, 0x25, 0xfd, 0xff, 0x0a,
    0x00, 0x14, 0x39, 0x02, 0x2d, 0x00, 0x00, 0x00
};
uint8_t frameRightMover[24] = {
    0x32, 0x30, 0x34, 0x04, 0x2b, 0x0a, 0x24, 0x18,
    0x02, 0x0a, 0x00, 0x33, 0x25, 0x82, 0x01, 0x0a,
    0x00, 0x14, 0x39, 0x02, 0x2d, 0x00, 0x00, 0x00
};

/* Exact PDB menu-definition globals at matched-PC RVAs 0x4C8D30 and
 * 0x53A0B0. menu_scoreComboDraw fills comboTotal[1] before interpreting
 * these one-command streams. */
uint32_t comboType[2] = {6, 0};
uint32_t comboTotal[2];

/* Exact initialized PDB globals at matched-PC RVAs 0x4C8D48..0x4C8D73. */
uint16_t redline[8] = {72, 298, 101, 287, 130, 276, 0, 0};
uint16_t barTable[4] = {0, 47, 76, 105};
uint16_t bonusMessBits[10] = {
    0x1000, 0x2000, 0x4000, 0x0080, 0x0100,
    0x0200, 0x0400, 0x0010, 0x0020, 0x0040
};
uint32_t comboDispDef[4] = {0x1d, 0x12, 4, 0x14};
uint32_t healthForceMdef[22] = {
    0, 2, 0x20, 0, 7, 0x21, 6, 0,
    0x1d, 0x12, 2, 8, 0, 0x11c, 0x7d,
    0x12, 3, 8, 1, 0x11d, 0x7f, 0x14
};
uint32_t eulaMdef[4] = {0, 0, 0x49, 0x14};

/* Exact PDB global at matched-PC RVA 0x4C6B30. */
uint32_t memoryMdef[24] = {
    0, 1, 3, 0x14, 0x38, 6, 0, 8,
    0, 0x50, 0x32, 3, 0x14, 0x24, 6, 0,
    8, UINT32_MAX, 0x50, 0x32, 0x10, 0x14, 0, 0
};

/* Exact PDB global at matched-PC RVA 0x4C75A8. */
uint32_t insert1Mdef[14] = {
    0, 1, 3, 0x100, 0x3c, 6, 2,
    8, 0, 0x12e, 0x32, 0x11, 0x14, 0
};

/* Exact PDB global at matched-PC RVA 0x4C7498. */
uint32_t loadMenuMdef[18] = {
    0, 1, 3, 0x100, 0x3c, 6, 2, 1, 9,
    0, 0x176, 0x52, 0x29, 0x11, 0x14, 0, 0, 0
};

/* Exact PDB globals at matched-PC RVAs 0x4C6A90 and 0x4C7AB0..0x4C817F. */
uint32_t memcarddebugMdef[40] = {
    0, 5, 3, 0x14, 0x28, 6, 0, 9, 0, 0xa8,
    0x32, 0x27, 8, 1, 0x51, 0x71, 8, 2, 0x52, 0x72,
    8, 3, 0x53, 0x73, 8, 4, 0x54, 0x74, 3, 0x3c0,
    0x14, 6, 2, 8, UINT32_MAX, 0xa8, 0x32, 0x10, 0x14, 0
};
uint32_t movieMenuMdef[24] = {
    0, 1, 3, 0x100, 0x28, 6, 2, 9,
    0, 0xbd, 0x39, 0x19, 6, 2, 3, 0x100,
    0x10, 8, UINT32_MAX, 0xbd, 0x32, 0x10, 0x14, 0
};
uint32_t rusureMenuMdef[28] = {
    0, 2, 0x3f, 0, 0x5a, 6, 2, 8,
    0, 0x110, 0x53, 8, 1, 0x111, 0x54, 0x3f,
    0, 0xd2, 8, UINT32_MAX, 0x11b, 0x32, 0x45, 0xdc,
    0x14, 0, 0, 0
};
uint32_t aidebugMenuMdef[84] = {
    6, 0, 0x42, 0, 3, 0x64, 0x64, 0, 0xe, 9,
    0, 0xd, 0x32, 0xc, 9, 1, 0x13, 0x32, 0x1d, 9,
    2, 0x40, 0x32, 0x1f, 9, 3, 0x41, 0x32, 0x20, 9,
    4, 0x43, 0x32, 0x21, 9, 5, 0x44, 0x32, 0x22, 9,
    6, 0x45, 0x32, 0x23, 9, 7, 0x46, 0x32, 0x24, 9,
    8, 0x47, 0x32, 0x25, 8, 9, 0x50, 0x31, 8, 0xa,
    0x55, 0x75, 8, 0xb, 0x5c, 0x79, 9, 0xc, 0xf, 0x32,
    0x2d, 0x42, 1, 3, 0, 0x64, 8, 0xd, 0x11, 0x29,
    0x10, 0x45, 0xdc, 0x14
};
uint32_t editMenuMdef[28] = {
    6, 0, 0x42, 0, 3, 0x64, 0x64, 0,
    3, 8, 0, 0x3a, 0x5f, 8, 1, 0x3b,
    0x48, 8, 2, 0x3c, 0x49, 0x10, 0x45, 0xdc,
    0x14, 0, 0, 0
};
uint32_t cameraMenuMdef[44] = {
    0, 6, 3, 0x100, 0x28, 6, 2, 8, 0, 0x3d,
    0x5f, 8, 1, 0xec, 0x60, 8, 2, 0xed, 0x61, 8,
    3, 0xee, 0x62, 8, 4, 0x11a, 0x63, 8, 5, 0x3e,
    0x64, 6, 2, 3, 0x100, 0x10, 8, UINT32_MAX, 0x19, 0x32,
    0x10, 0x45, 0x1b1, 0x14
};
uint32_t objectiveMenuMdef[44] = {
    0, 1, 0x3f, 0, 0, 6, 2, 0xf, 0, 2, 6,
    0x40, 0x40, 0x236, 0x122, 9, UINT32_MAX, 0xe0, 0x32, 7,
    6, 0, 0x40, 0x236, 0xd2, 9, UINT32_MAX, 2, 0x32, 0x1e,
    6, 0, 0x3e, 0x236, 0x5a, 8, UINT32_MAX, 0xdd, 0x32, 0x46,
    0xdc, 0x14, 0, 0
};
uint32_t specialMessMenuMdef[24] = {
    0, 1, 0xf, 0, 2, 6, 0x40, 0x40,
    0x1f4, 0x122, 0x1a, UINT32_MAX, 6, 0, 0x3e, 0x1f4,
    0x5a, 8, UINT32_MAX, 0xdd, 0x32, 0x45, 0xdc, 0x14
};
uint32_t specialMessMenu2Mdef[24] = {
    0, 1, 0xf, 0, 2, 6, 0x40, 0x40,
    0x1f4, 0x122, 0x1a, UINT32_MAX, 6, 0, 0x3e, 0x1f4,
    0x5a, 8, UINT32_MAX, 0xdd, 0x32, 0x45, 0xdc, 0x14
};
uint32_t gamecombosMenu[12] = {
    0, 1, 8, 0, 2, 0x4e, 0x10, 0x47, 0x14, 0, 0, 0
};

/* Exact PDB global at matched-PC RVA 0x4C8260. */
uint32_t ngpUnlockMdef[26] = {
    0, 1, 0xf, 0, 2, 6, 0, 0x40, 0x212, 0x50,
    8, UINT32_MAX, 0x1e8, 0x32, 6, 0, 0x3e, 0x212,
    0x118, 8, UINT32_MAX, 0xdd, 0x32, 0x45, 0x1b1, 0x14
};

/* Exact PDB globals at matched-PC RVAs 0x4C7CC0 and 0x4C8CC0..0x4C8CF3. */
uint32_t gamepauseMenuMdef[48] = {
    0, 6, 0x3f, 0, 0xd2, 6, 2, 8,
    0, 0x102, 0x3f, 8, 1, 0xf2, 0x23, 8,
    2, 0xff, 0x10, 8, 3, 0xfe, 0x11, 0x12,
    6, 9, 4, 0x179, 0x32, 0x37, 8, 5,
    0x103, 0x2e, 6, 2, 0x3f, 0, 0x122, 8,
    UINT32_MAX, 0x18d, 0x32, 0x45, 0xdc, 0x14, 0, 0
};
unsigned short cheatCheckPoint[10] = {
    4, 8, 4, 4, 8, 8, 4, 8, 8, 4
};
unsigned char cheatCheckPointKeyboard[10] = {
    'P', 'O', 'P', 'P', 'O', 'O', 'P', 'O', 'O', 'P'
};
unsigned short cheatRadar[6] = {
    0x1000, 0x4000, 0x1000, 4, 8, 4
};

/* Exact PDB globals at matched-PC RVAs 0x4C68D0 and 0x4C7FB0. */
uint32_t debugMdef[112] = {
    0, 0x13, 3, 0x32, 0x50, 6, 0, 0xc,
    4, 9, 0, 0xbe, 0x3e, 6, 9, 1,
    0xe4, 0x32, 1, 9, 2, 0xe7, 0x32, 4,
    9, 3, 0xe8, 0x32, 5, 8, 4, 0x3f,
    0x27, 9, 5, 0x4f, 0x32, 0x33, 8, 6,
    0x50, 0x31, 8, 7, 0xa8, 0x30, 9, 8,
    0xf, 0x32, 0x2d, 8, 9, 0x10, 0x32, 8,
    0xa, 0xb9, 0x1d, 8, 0xb, 0xb8, 0x26, 3,
    0x140, 0x50, 9, 0xc, 0xe7, 0x32, 0, 9,
    0xd, 0xe8, 0x32, 0, 9, 0xe, 0x7b, 0x38,
    0x35, 9, 0xf, 0x7c, 0x32, 0x36, 9, 0x10,
    0xbd, 0x32, 0x38, 8, 0x11, 0xa8, 8, 9,
    0x12, 4, 0x33, 0x39, 3, 0x3c0, 0x2c, 6,
    2, 8, UINT32_MAX, 0x4f, 0x32, 0x10, 0x14, 0
};
uint32_t comboDebugMenuMdef[44] = {
    6, 0, 0x42, 0, 3, 0x64, 0x64, 0,
    6, 9, 0, 0x42, 0x32, 0x31, 9, 1,
    0xe7, 0x32, 0, 9, 2, 0xe8, 0x32, 0,
    9, 3, 0x65, 0x7a, 0x32, 8, 4, 0x66,
    0x7b, 8, 5, 0x67, 0x7c, 0x10, 0x45, 0xdc,
    0x14, 0, 0, 0
};

/* Exact PDB table at matched-PC RVA 0x4C5C20. */
const ScoreScreenModelMap psxScoreScreenMaps[23] = {
    {obi_wan_model, "obi2"},
    {qui_gon_model, "qui2"},
    {mace_model, "mace2"},
    {adi_model, "adi2"},
    {plo_model, "plo2"},
    {ki_adi_model, "Ki_Adi_Score"},
    {maul_p_model, "maul"},
    {amidala_model, "queen"},
    {panaka_model, "panaka"},
    {jar_jar_playable_model, "JarJar_Score"},
    {battle_d_model, "BattleDroidMelee_Score"},
    {pilot_model, "Pilot_Score"},
    {rifle_model, "BattleDroidRifle_Score"},
    {flame_model, "FlameDroid_Score"},
    {destroye_model, "Destroyer_Score"},
    {loader_model, "Loader_Score"},
    {tusken_s_model, "TuskenStaff_Score"},
    {tusken_r_model, "TuskenRifle_Score"},
    {thug_1_model, "Weequay_Score"},
    {thug_2_model, "IshiTib_Score"},
    {thug_3_model, "Rodian_Score"},
    {thug_4_model, "Merc_Score"},
    {gungan_1_model, "GunganGuard_Score"}
};

/* Exact PDB menu definitions and pointer table at RVAs 0x4C7620 and
 * 0x4C77F8..0x4C8357. */
uint32_t saveNowMdef[26] = {
    0, 2, 0x20, 1, 0, 3, 0x100, 0x78,
    6, 2, 8, 0, 0x111, 0x1d, 8, 1,
    0x110, 0x1f, 3, 0x100, 0x5a, 8, UINT32_MAX, 0x131,
    0x32, 0x14
};
uint32_t saveNowSureMdef[26] = {
    0, 2, 0x20, 1, 0, 3, 0x100, 0x78,
    6, 2, 8, 0, 0x110, 0x4e, 8, 1,
    0x111, 0x4f, 3, 0x100, 0x5a, 8, UINT32_MAX, 0x17c,
    0x32, 0x14
};
uint32_t frameTopMdef[7] = {2, 0, 0, 5, 0x1e, 3, 0x14};
uint32_t frameBotMdef[7] = {2, 0, 0, 5, 0x1e, 2, 0x14};
uint32_t frameBotMdefls[8] = {2, 0, 0, 5, 0x1f, 2, 0xc8, 0x14};
uint32_t frameLeftMdef[7] = {2, 0, 0, 5, 0x1e, 4, 0x14};
uint32_t frameRightMdef[7] = {2, 0, 0, 5, 0x1e, 5, 0x14};
uint32_t *moverMenus[6] = {
    frameTopMdef,
    frameBotMdef,
    frameBotMdefls,
    frameLeftMdef,
    frameRightMdef,
    saveNowMdef
};

/* Exact PDB pointer table at RVA 0x4BDB00. */
char *menu_soundList[11] = {
    "",
    "xjedscrl",
    "xopt_sel",
    "xjedscrl",
    "xjedsel",
    "xlvbrows",
    "xlvselct",
    "xsecret",
    "xsavload",
    "xlocklvl",
    "xpointbp"
};

/*
 * Exact 132-entry menuTextureList image at matched-PC RVA 0x4CAC10.
 * Filenames and numeric fields are copied from game.exe; the final NULL
 * filename intentionally asks _LoadTexture for the default white material.
 */
const JPBMenuTextureEntry menuTextureList[JPB_MENU_TEXTURE_ENTRY_COUNT] = {
    {"winsys0.png", 0, 1, UINT32_C(0x00000010)},
    {"winif0.png", 1, 154, UINT32_C(0x00000010)},
    {"winif2.png", 3, 232, UINT32_C(0x00000010)},
    {"JPB_SplashV3_Sharpened.png", 4, 346, UINT32_C(0x00000080)},
    {"JPB_SplashV3_Sharpened.png", 5, 347, UINT32_C(0x00000080)},
    {"art_01.png", 121, 352, UINT32_C(0xFFFFFFFF)},
    {"art_02.png", 122, 352, UINT32_C(0xFFFFFFFF)},
    {"art_03.png", 123, 352, UINT32_C(0xFFFFFFFF)},
    {"art_04.png", 124, 352, UINT32_C(0xFFFFFFFF)},
    {"art_05.png", 125, 352, UINT32_C(0xFFFFFFFF)},
    {"art_06.png", 126, 352, UINT32_C(0xFFFFFFFF)},
    {"art_07.png", 127, 352, UINT32_C(0xFFFFFFFF)},
    {"art_09.png", 128, 352, UINT32_C(0xFFFFFFFF)},
    {"art_10.png", 129, 352, UINT32_C(0xFFFFFFFF)},
    {"art_11.png", 130, 352, UINT32_C(0xFFFFFFFF)},
    {"art_12.png", 131, 352, UINT32_C(0xFFFFFFFF)},
    {"art_13.png", 132, 352, UINT32_C(0xFFFFFFFF)},
    {"art_14.png", 133, 352, UINT32_C(0xFFFFFFFF)},
    {"art_15.png", 134, 352, UINT32_C(0xFFFFFFFF)},
    {"art_16.png", 135, 352, UINT32_C(0xFFFFFFFF)},
    {"art_18.png", 136, 352, UINT32_C(0xFFFFFFFF)},
    {"art_19.png", 137, 352, UINT32_C(0xFFFFFFFF)},
    {"art_20.png", 138, 352, UINT32_C(0xFFFFFFFF)},
    {"art_21.png", 139, 352, UINT32_C(0xFFFFFFFF)},
    {"art_22.png", 140, 352, UINT32_C(0xFFFFFFFF)},
    {"art_23.png", 141, 352, UINT32_C(0xFFFFFFFF)},
    {"art_24.png", 142, 352, UINT32_C(0xFFFFFFFF)},
    {"art_25.png", 143, 352, UINT32_C(0xFFFFFFFF)},
    {"art_27.png", 144, 352, UINT32_C(0xFFFFFFFF)},
    {"art_28.png", 145, 352, UINT32_C(0xFFFFFFFF)},
    {"art_29.png", 146, 352, UINT32_C(0xFFFFFFFF)},
    {"art_30.png", 147, 352, UINT32_C(0xFFFFFFFF)},
    {"art_31.png", 148, 352, UINT32_C(0xFFFFFFFF)},
    {"art_32.png", 149, 352, UINT32_C(0xFFFFFFFF)},
    {"art_33.png", 150, 352, UINT32_C(0xFFFFFFFF)},
    {"art_34.png", 151, 352, UINT32_C(0xFFFFFFFF)},
    {"art_35.png", 152, 352, UINT32_C(0xFFFFFFFF)},
    {"art_36.png", 153, 352, UINT32_C(0xFFFFFFFF)},
    {"art_37.png", 154, 352, UINT32_C(0xFFFFFFFF)},
    {"art_38.png", 155, 352, UINT32_C(0xFFFFFFFF)},
    {"art_39.png", 156, 352, UINT32_C(0xFFFFFFFF)},
    {"art_40.png", 157, 352, UINT32_C(0xFFFFFFFF)},
    {"art_41.png", 158, 352, UINT32_C(0xFFFFFFFF)},
    {"art_42.png", 159, 352, UINT32_C(0xFFFFFFFF)},
    {"art_43.png", 160, 352, UINT32_C(0xFFFFFFFF)},
    {"art_44.png", 161, 352, UINT32_C(0xFFFFFFFF)},
    {"art_46.png", 162, 352, UINT32_C(0xFFFFFFFF)},
    {"loadscreenbg.png", 164, 352, UINT32_C(0x00000010)},
    {"loadscreenfg.png", 165, 352, UINT32_C(0x00000010)},
    {"loadbar_gradient.png", 166, 352, UINT32_C(0x00000010)},
    {"loadbar_mask.png", 167, 352, UINT32_C(0x00000010)},
    {"loadbar_background.png", 168, 352, UINT32_C(0x00000010)},
    {"NewUI/Arrow_Pressed_Left.png", 169, 352, UINT32_C(0x00000010)},
    {"NewUI/Arrow_Pressed_Right.png", 170, 352, UINT32_C(0x00000010)},
    {"NewUI/Arrow_Selected.png", 171, 352, UINT32_C(0x00000010)},
    {"NewUI/Arrow_Unselected_Left.png", 172, 352, UINT32_C(0x00000010)},
    {"NewUI/Arrow_Unselected_Right.png", 173, 352, UINT32_C(0x00000010)},
    {"NewUI/blasterIcon.png", 174, 352, UINT32_C(0x00000010)},
    {"NewUI/BlueBackground.png", 175, 352, UINT32_C(0x00000010)},
    {"NewUI/fistIcon.png", 176, 352, UINT32_C(0x00000010)},
    {"NewUI/GrayBar_Divider.png", 177, 352, UINT32_C(0x00000010)},
    {"NewUI/Lightsaber_Blue.png", 178, 352, UINT32_C(0x00000010)},
    {"NewUI/Lightsaber_Green.png", 179, 352, UINT32_C(0x00000010)},
    {"NewUI/Lightsaber_Purple.png", 180, 352, UINT32_C(0x00000010)},
    {"NewUI/Lightsaber_Red.png", 181, 352, UINT32_C(0x00000010)},
    {"NewUI/Lightsaber_Yellow.png", 182, 352, UINT32_C(0x00000010)},
    {"NewUI/PlayerContainer.png", 183, 352, UINT32_C(0x00000010)},
    {"NewUI/PlayerContainer_Blue.png", 184, 352, UINT32_C(0x00000010)},
    {"NewUI/PlayerNameBar.png", 185, 352, UINT32_C(0x00000010)},
    {"NewUI/PlayerNameBar_570.png", 186, 352, UINT32_C(0x00000010)},
    {"NewUI/PlayerNameBar_610.png", 187, 352, UINT32_C(0x00000010)},
    {"NewUI/SinglePlayerTopBar.png", 188, 352, UINT32_C(0x00000010)},
    {"NewUI/SkillContainer.png", 189, 352, UINT32_C(0x00000010)},
    {"NewUI/TwoPlayerTopBar_Half.png", 190, 352, UINT32_C(0x00000010)},
    {"NewUI/WhiteBar_Detail.png", 191, 352, UINT32_C(0x00000010)},
    {"NewUI/WhiteBars_Details.png", 192, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Pressed_Left_Small.png", 193, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Pressed_Right_Small.png", 194, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Pressed_Up_Small.png", 195, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Pressed_Down_Small.png", 196, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Unselected_Left_Small.png", 197, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Unselected_Right_Small.png", 198, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Unselected_Up_Small.png", 199, 352, UINT32_C(0x00000010)},
    {"NewUI/SmallArrow/Arrow_Unselected_Down_Small.png", 200, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Obi.png", 201, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Qui.png", 202, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Mace.png", 203, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Adi.png", 204, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Plo.png", 205, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Maul.png", 206, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Queen.png", 207, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Panaka.png", 208, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Ki_Adi.png", 209, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Pilot.png", 210, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/BattleDroidMelee.png", 211, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/BattleDroidRifle.png", 212, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/FlameDroid.png", 213, 352, UINT32_C(0x00000010)},
    {"NewUI/BlueBackground.png", 214, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Destroyer.png", 215, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Loader.png", 216, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/TuskenStaff.png", 217, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/TuskenRifle.png", 218, 352, UINT32_C(0x00000010)},
    {"NewUI/BlueBackground.png", 219, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Weequay.png", 220, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/IshiTib.png", 221, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Rodian.png", 222, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/Merc.png", 223, 352, UINT32_C(0x00000010)},
    {"NewUI/BlueBackground.png", 224, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/GunganGuard.png", 225, 352, UINT32_C(0x00000010)},
    {"NewUI/CharacterSelectImages/JarJar.png", 226, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train1.png", 227, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train2.png", 228, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train3.png", 229, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train4.png", 230, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train5.png", 231, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train6.png", 232, 352, UINT32_C(0x00000010)},
    {"TrainingImages/Train7.png", 233, 352, UINT32_C(0x00000010)},
    {"NewUI/LevelSelect/comboBox_Bottom.png", 234, 352, UINT32_C(0x00000010)},
    {"NewUI/LevelSelect/comboHeader.png", 235, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/Large_PopUp_Box.png", 236, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/Small_PopUp_Box.png", 237, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/Small_PopUp_Box_1250.png", 238, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/confirm_Box_green.png", 239, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/confirm_Box_650.png", 240, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/confirm_Box_750.png", 241, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/eula_Box.png", 242, 352, UINT32_C(0x00000010)},
    {"PopUpBoxes/eula_Box_Slider.png", 243, 352, UINT32_C(0x00000010)},
    {"mainMenuTextBox.png", 244, 352, UINT32_C(0x00000010)},
    {"scorescreen/ExtraElements/score_Gradient.png", 246, 352, UINT32_C(0x00000010)},
    {"scorescreen/ExtraElements/score_GreenLine.png", 247, 352, UINT32_C(0x00000010)},
    {"scorescreen/ExtraElements/score_WhiteLine.png", 248, 352, UINT32_C(0x00000010)},
    {NULL, 119, 449, UINT32_C(0x00000010)},
};

_Static_assert(
    sizeof(JPBMenuTextureEntry) == 16,
    "menuTextureList record layout changed");

/*
 * Exact 74-entry modifier table at matched-PC RVA 0x4C8360. Pointer values
 * are rebound to the corresponding recovered PDB globals and struct fields;
 * all scalar metadata is copied directly from game.exe.
 */
MDEF_MOD modVars[74] = {
    JPB_MOD(0x0000, 1, 0, 0, NULL, 2),
    JPB_MOD(0x0000, 1, 1, 2, &GameStruct.NumPlayers, 2),
    JPB_MOD(0x8000, 1, 0, 22, &menuVars.pplayers[0], 2),
    JPB_MOD(0x8000, 1, 0, 22, &menuVars.pplayers[1], 2),
    JPB_MOD(0x8000, 1, 0, 79, &GameStruct.ModelSelect[0], 0x8000),
    JPB_MOD(0x8000, 1, 0, 79, &GameStruct.ModelSelect[1], 0x8000),
    JPB_MOD(0x8000, 1, 1, 29, &LevelSelect, 0x8001),
    JPB_MOD(0x8000, 1, 1, 29, &LevelSelect, 305),
    JPB_MOD(0x8000, 1, 1, 14, &LevelSelect, 2),
    JPB_MOD(0x8000, 1, 1, 25, &menuVars.dialogBox1, 0x8001),
    JPB_MOD(0x8000, 1, 0, 3, &OptionStruct.CPULevel, 26),
    JPB_MOD(0x8000, 1, 0, 8, &OptionStruct.FunFactor, 49),
    JPB_MOD(0x8000, 1, 0, 15, &OptionStruct.AIDebug, 33),
    JPB_MOD(0x0000, 1, 0, 3, &OptionStruct.DebugLevel, 2),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.ControllerConfig[0], 277),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.ControllerConfig[1], 277),
    JPB_MOD(0x2800, 1, 0, 8, &OptionStruct.WalkLimit[0], 2),
    JPB_MOD(0x2800, 1, 0, 8, &OptionStruct.WalkLimit[1], 2),
    JPB_MOD(0x2800, 1, 0, 8, &OptionStruct.RunLimit[0], 2),
    JPB_MOD(0x2800, 1, 0, 8, &OptionStruct.RunLimit[1], 2),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.Music, 277),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.Stereo, 280),
    JPB_MOD(0x3801, 3, 0, 75, &OptionStruct.musicVolume, 2),
    JPB_MOD(0x3801, 3, 0, 75, &OptionStruct.SFXVolume, 2),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.PadAudioEnabled, 277),
    JPB_MOD(0x8000, 1, 0, 7, &menuVars.movieSelect, 93),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.ControllerConfig[0], 492),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.ControllerConfig[1], 492),
    JPB_MOD(0x8000, 1, 0, 1, &GameStruct.QuickDraw, 30),
    JPB_MOD(0x8000, 1, 0, 1, &OptionStruct.JumpCheat, 278),
    JPB_MOD(0x8000, 1, 0, 1000, &LevelSelect, 191),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.AIDamage, 49),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.JediDamage, 49),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.HTHRate, 49),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.RangedRate, 49),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.BlockRate, 49),
    JPB_MOD(0x0000, 1, 0, 17, &GameStruct.ComboLevel, 49),
    JPB_MOD(0x0000, 1, 0, 15, &GameStruct.ForceLevel, 49),
    JPB_MOD(0x0000, 1, 0, 101, &OptionStruct.xaTrack, 2),
    JPB_MOD(0x8001, 1, 0, 1, &menuVars.cardSelect, 267),
    JPB_MOD(0x4001, 1, 0, 1, &menuVars.cardSelect, 0),
    JPB_MOD(0x8001, 1, 0, 2, &menuVars.cardSlotSelect, 267),
    JPB_MOD(0x0001, 1, 0, 63, &menuVars.aibit, 2),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.comboSelect, 90),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.pauseMenu, 272),
    JPB_MOD(0x8000, 1, 0, 1, &GameStruct.screenShotFlag, 277),
    JPB_MOD(0x8000, 1, 0, 1, &GameStruct.timerBars, 277),
    JPB_MOD(0x8000, 1, 1, 64, &GameStruct.maxdraw, 277),
    JPB_MOD(0x0000, 1, 0, 15, &menuVars.controlFlags, 2),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.savePosSlot, 237),
    JPB_MOD(0x0000, 1, 0, 47, &menuVars.jediDebugCombo, 2),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.gameDebugMode, 278),
    JPB_MOD(0x8000, 1, 0, 12, &menuVars.gmiNum, 110),
    JPB_MOD(0x0000, 1, 0, 10, &menuVars.sndtest, 2),
    JPB_MOD(0x0003, 1, 0, 459, &menuVars.textures, 2),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.ultimate, 277),
    JPB_MOD(0x8000, 1, 0, 1, &menuVars.ingameMovies, 278),
    JPB_MOD(0x0001, 1, 0, 15, &menuVars.sbit, 2),
    JPB_MOD(0x0001, 1, 0, 8, &menuVars.trainingLevel, 2),
    JPB_MOD(0x8000, 1, 0, 1, &GameStruct.AIselect[0], 383),
    JPB_MOD(0x8000, 1, 0, 1, &GameStruct.AIselect[1], 383),
    JPB_MOD(0x0004, 1, 0, 8, &GameStruct.Counter, 2),
    JPB_MOD(0x0001, 1, 0, 1, &camerablockactive, 2),
    JPB_MOD(0x0001, 1, 0, 1, &oldworld, 2),
    JPB_MOD(0x0001, 1, 0, 1, &alwaysRun, 2),
    JPB_MOD(0x0001, 1, 0, 255, &dimScreen, 2),
    JPB_MOD(0x0001, 1, 0, 255, &brightMax, 2),
    JPB_MOD(0x0002, 1, 0, 1, &GameStruct.versusModeFlag, 2),
    JPB_MOD(0x0001, 1, 0, 1, &streets, 2),
    JPB_MOD(0x8000, 1, 0, 1, NULL, 265),
    JPB_MOD(0x0001, 1, 0, 1, &nextLevel, 2),
    JPB_MOD(0x8000, 1, 0, 6, &OptionStruct.Language, 147),
    JPB_MOD(0x8001, 1, 0, 2, &OptionStruct.WindowMode, 446),
    JPB_MOD(0x0006, 1, 0, 256, &OptionStruct.ResolutionChanged, 2),
};

/* Exact 74 names and null sentinel at matched-PC RVA 0x4CA790. */
char *modVarNames[75] = {
    "nada_mod", "players", "p1sel", "p2sel", "player1", "player2",
    "level", "fulllevel", "level2", "tlevel", "diff", "fun", "ai",
    "debuglevel", "shock1", "shock2", "walklim1", "walklim2",
    "runlim1", "runlim2", "music", "STEREO", "musicvol", "sfxvol",
    "conspkr", "moviesel", "control1", "control2", "qkdraw", "jumpy",
    "obj", "aidam", "jedidam", "hthrate", "rangedrate", "blockrate",
    "pooplvl", "forcelvl", "xatrack", "memcrd", "memcrd2",
    "memcrdslot", "aibit", "pausemenu", "dialogbox1", "screenshot",
    "timerbars", "maxdraw", "saveposslot", "jedicombo", "combosel",
    "gamedbmode", "gmi", "soundtest", "textures", "ultimate",
    "ingamemovie", "sbit", "training", "ai1", "ai2", "rescue",
    "camerablock", "obi", "runnin", "dimscreen", "maxbright", "versus",
    "streets", "freq", "nextlvl", "language", "resmode", "ressize",
    NULL
};

#undef JPB_MOD

uint32_t xmainMdef[43] = {
    UINT32_C(0x00000000), UINT32_C(0x00000003), UINT32_C(0x00000003), UINT32_C(0x00000100), UINT32_C(0x000000A5), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x0000000E), UINT32_C(0x00000008),
    UINT32_C(0x00000000), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B8),
    UINT32_C(0x00000026), UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000100), UINT32_C(0x0000002D),
    UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B8), UINT32_C(0x00000000), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF),
    UINT32_C(0x000000BC), UINT32_C(0x00000000), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000000D), UINT32_C(0x00000001),
    UINT32_C(0x00000014),
};

uint32_t mainMdef[65] = {
    UINT32_C(0x00000000), UINT32_C(0x00000006), UINT32_C(0x00000003), UINT32_C(0x00000000), UINT32_C(0x000000B9), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x00000008), UINT32_C(0x00000000),
    UINT32_C(0x000000B6), UINT32_C(0x00000090), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x0000014B), UINT32_C(0x0000000C),
    UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000003),
    UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008), UINT32_C(0x00000004), UINT32_C(0x00000103), UINT32_C(0x00000092),
    UINT32_C(0x00000008), UINT32_C(0x00000005), UINT32_C(0x00000190), UINT32_C(0x0000008F), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF),
    UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000041), UINT32_C(0x00000000), UINT32_C(0x00000038), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000BC),
    UINT32_C(0x0000000B), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x00000103), UINT32_C(0x00000092), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x00000190), UINT32_C(0x0000008F), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000003E),
    UINT32_C(0x000001A4), UINT32_C(0x0000008A), UINT32_C(0x00000044), UINT32_C(0x00000010), UINT32_C(0x00000014),
};

uint32_t mainMdefNoRegisterGame[57] = {
    UINT32_C(0x00000000), UINT32_C(0x00000005), UINT32_C(0x00000003), UINT32_C(0x00000000), UINT32_C(0x000000B9), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x00000008), UINT32_C(0x00000000),
    UINT32_C(0x000000B6), UINT32_C(0x00000090), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x0000014B), UINT32_C(0x0000000C),
    UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000003),
    UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008), UINT32_C(0x00000004), UINT32_C(0x00000103), UINT32_C(0x00000092),
    UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000041), UINT32_C(0x00000000),
    UINT32_C(0x00000004), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x00000103),
    UINT32_C(0x00000092), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000003E), UINT32_C(0x000001A4), UINT32_C(0x0000008A),
    UINT32_C(0x00000044), UINT32_C(0x00000010), UINT32_C(0x00000014),
};

uint32_t NOLOADmainMdef[24] = {
    UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000003), UINT32_C(0x000003C0), UINT32_C(0x00000096), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000008), UINT32_C(0x00000000), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008),
    UINT32_C(0x00000001), UINT32_C(0x0000014B), UINT32_C(0x0000000C), UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000017D),
    UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000003), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000014),
};

uint32_t PSXmainMdef[51] = {
    UINT32_C(0x00000000), UINT32_C(0x00000003), UINT32_C(0x00000003), UINT32_C(0x00000100), UINT32_C(0x000000A5), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x0000000E), UINT32_C(0x00000008),
    UINT32_C(0x00000000), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B8),
    UINT32_C(0x00000032), UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000100), UINT32_C(0x0000002D),
    UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B8), UINT32_C(0x00000000), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF),
    UINT32_C(0x000000BC), UINT32_C(0x00000000), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000000D), UINT32_C(0x00000001),
    UINT32_C(0x00000003), UINT32_C(0x0000021A), UINT32_C(0x0000010E), UINT32_C(0x0000003E), UINT32_C(0x000001A4), UINT32_C(0x0000008A),
    UINT32_C(0x00000044), UINT32_C(0x00000010), UINT32_C(0x00000014),
};

uint32_t continuemainMdef[73] = {
    UINT32_C(0x00000000), UINT32_C(0x00000007), UINT32_C(0x00000003), UINT32_C(0x00000000), UINT32_C(0x000000B9), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x00000008), UINT32_C(0x00000000),
    UINT32_C(0x000000B7), UINT32_C(0x0000000A), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B6), UINT32_C(0x00000090),
    UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000014B), UINT32_C(0x0000000C), UINT32_C(0x00000008), UINT32_C(0x00000003),
    UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000004), UINT32_C(0x000000BC), UINT32_C(0x0000000B),
    UINT32_C(0x00000008), UINT32_C(0x00000005), UINT32_C(0x00000103), UINT32_C(0x00000092), UINT32_C(0x00000008), UINT32_C(0x00000006),
    UINT32_C(0x00000190), UINT32_C(0x0000008F), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B7), UINT32_C(0x0000000A),
    UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000041), UINT32_C(0x00000000),
    UINT32_C(0x00000038), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x00000103),
    UINT32_C(0x00000092), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x00000190), UINT32_C(0x0000008F), UINT32_C(0x00000016),
    UINT32_C(0x00000002), UINT32_C(0x0000003E), UINT32_C(0x000001A4), UINT32_C(0x0000008A), UINT32_C(0x00000044), UINT32_C(0x00000010),
    UINT32_C(0x00000014),
};

uint32_t continuemainMdefNoRegisterGame[65] = {
    UINT32_C(0x00000000), UINT32_C(0x00000006), UINT32_C(0x00000003), UINT32_C(0x00000000), UINT32_C(0x000000B9), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x00000008), UINT32_C(0x00000000),
    UINT32_C(0x000000B7), UINT32_C(0x0000000A), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B6), UINT32_C(0x00000090),
    UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000014B), UINT32_C(0x0000000C), UINT32_C(0x00000008), UINT32_C(0x00000003),
    UINT32_C(0x0000017D), UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000004), UINT32_C(0x000000BC), UINT32_C(0x0000000B),
    UINT32_C(0x00000008), UINT32_C(0x00000005), UINT32_C(0x00000103), UINT32_C(0x00000092), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF),
    UINT32_C(0x000000B7), UINT32_C(0x0000000A), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000009),
    UINT32_C(0x00000041), UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x0000017D),
    UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x00000103), UINT32_C(0x00000092), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000003E),
    UINT32_C(0x000001A4), UINT32_C(0x0000008A), UINT32_C(0x00000044), UINT32_C(0x00000010), UINT32_C(0x00000014),
};

uint32_t NOLOADmainMdef2[24] = {
    UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000003), UINT32_C(0x000003C0), UINT32_C(0x00000096), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000008), UINT32_C(0x00000000), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008),
    UINT32_C(0x00000001), UINT32_C(0x0000014B), UINT32_C(0x0000000C), UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x0000017D),
    UINT32_C(0x0000000D), UINT32_C(0x00000008), UINT32_C(0x00000003), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000014),
};

uint32_t PSXmainMdef2[51] = {
    UINT32_C(0x00000000), UINT32_C(0x00000003), UINT32_C(0x00000003), UINT32_C(0x00000100), UINT32_C(0x000000A5), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000015), UINT32_C(0x00000016), UINT32_C(0x00000001), UINT32_C(0x0000000E), UINT32_C(0x00000008),
    UINT32_C(0x00000000), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B8),
    UINT32_C(0x00000032), UINT32_C(0x00000008), UINT32_C(0x00000002), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000008),
    UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B6), UINT32_C(0x00000000), UINT32_C(0x00000004), UINT32_C(0x00000100), UINT32_C(0x0000002D),
    UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF), UINT32_C(0x000000B8), UINT32_C(0x00000000), UINT32_C(0x00000008), UINT32_C(0xFFFFFFFF),
    UINT32_C(0x000000BC), UINT32_C(0x00000000), UINT32_C(0x00000016), UINT32_C(0x00000002), UINT32_C(0x0000000D), UINT32_C(0x00000001),
    UINT32_C(0x00000003), UINT32_C(0x0000021A), UINT32_C(0x0000010E), UINT32_C(0x0000003E), UINT32_C(0x000001A4), UINT32_C(0x0000008A),
    UINT32_C(0x00000044), UINT32_C(0x00000010), UINT32_C(0x00000014),
};

uint32_t continueNOLOADmainMdef[30] = {
    UINT32_C(0x00000000), UINT32_C(0x00000005), UINT32_C(0x00000003), UINT32_C(0x000003C0), UINT32_C(0x00000096), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000012), UINT32_C(0x00000000), UINT32_C(0x00000008), UINT32_C(0x00000000), UINT32_C(0x000000B7),
    UINT32_C(0x0000000A), UINT32_C(0x00000008), UINT32_C(0x00000001), UINT32_C(0x000000B6), UINT32_C(0x00000009), UINT32_C(0x00000008),
    UINT32_C(0x00000002), UINT32_C(0x0000014B), UINT32_C(0x0000000C), UINT32_C(0x00000008), UINT32_C(0x00000003), UINT32_C(0x0000017D),
    UINT32_C(0x0000009D), UINT32_C(0x00000008), UINT32_C(0x00000004), UINT32_C(0x000000BC), UINT32_C(0x0000000B), UINT32_C(0x00000014),
};

/*
 * Exact first-level title submenus. Their extents come from the PDB array
 * types; the words come from matched-PC RVAs 0x4C66D0..0x4C68C4,
 * 0x4C6ED0..0x4C6F7C, and 0x4C7DF0..0x4C7E54.
 */
uint32_t titlePlayerCountMdef[21] = {
    0, 2, 3, 0, 154, 6, 2,
    8, 0, 233, 106,
    8, 1, 234, 107,
    0x3e, 420, 138, 0x44, 0x10, 0x14
};

uint32_t titlePlayerCountContinueMdef[21] = {
    0, 2, 3, 0, 154, 6, 2,
    8, 0, 233, 151,
    8, 1, 234, 152,
    0x3e, 420, 138, 0x44, 0x10, 0x14
};

uint32_t titlePlayerCountVSMdef[21] = {
    0, 2, 3, 0, 154, 6, 2,
    8, 0, 233, 154,
    8, 1, 234, 155,
    0x3e, 420, 138, 0x44, 0x10, 0x14
};

uint32_t difficultyMdef[21] = {
    0, 2, 3, 0, 154, 6, 2,
    8, 0, 363, 105,
    8, 1, 362, 104,
    0x3e, 420, 138, 0x44, 0x10, 0x14
};

/*
 * Exact PDB array at matched-PC RVA 0x4C6B90. This is the ordinary
 * command stream used by menu_mainLoop state 4 after the authored player
 * presentation has established its selection controls.
 */
uint32_t playerCountSelectMdef[20] = {
    0, 2, 3, 256, 160, 6, 2,
    8, 0, 233, 106,
    8, 1, 234, 107,
    0x10, 0x0b, 0x0a, 0x14, 0
};

uint32_t newgameconfirmMdef[29] = {
    0, 2, 3, 0, 175, 6, 2,
    8, 0, 273, 9,
    8, 1, 272, 83,
    3, 0, 85,
    8, UINT32_MAX, 180, 50,
    3, 0, 0, 0x48, 585, 0x10, 0x14
};

uint32_t optionsMdef[44] = {
    0, 6, 3, 0, 0, 6, 2,
    8, 0, 462, 149,
    8, 1, 255, 16,
    8, 2, 242, 35,
    8, 3, 145, 145,
    8, 4, 290, 19,
    8, 5, 378, 27,
    6, 2, 0x3f, 0, 80,
    8, UINT32_MAX, 188, 50,
    0x10, 0x45, 433, 0x14
};

/*
 * Exact PDB-named control, language, and video option streams at matched-PC
 * RVAs 0x4C6F80..0x4C7278. The PDB array types establish every extent.
 */
uint32_t controlsMdef[16] = {
    0, 2, 0x3f, 0, 500, 6, 2,
    8, 0, 237, 141,
    8, 1, 238, 142,
    0x14
};

uint32_t titlecontrolsMdef[16] = {
    0, 2, 0x3f, 0, 500, 6, 2,
    8, 0, 237, 141,
    8, 1, 238, 142,
    0x14
};

uint32_t controls1Mdef[42] = {
    0, 5, 0x40, 685, 125, 6, 0,
    0x4a, 1, 0x1c, 1,
    9, 0, 244, 50, 26,
    0x12, 5,
    9, 1, 256, 50, 14,
    0x12, 5,
    9, 2, 370, 50, 16,
    0x12, 5,
    9, 3, 371, 50, 18,
    8, 4, 257, 103,
    0x14
};

uint32_t controls2Mdef[42] = {
    0, 5, 0x3f, 215, 125, 6, 0,
    0x4a, 1, 0x1c, 2,
    9, 0, 244, 50, 27,
    0x12, 5,
    9, 1, 256, 50, 15,
    0x12, 5,
    9, 2, 370, 50, 17,
    0x12, 5,
    9, 3, 371, 50, 19,
    8, 4, 257, 103,
    0x14
};

/*
 * Exact PDB-named Controls presentation data recovered from matched-PC RVAs
 * 0x4C6374, 0x4C6484, 0x4C6638, 0x4C66C8, 0x4C7158, and
 * 0x4C8CA0..0x4C8CBC. The byte mappings select an artwork slot for each
 * Classic/Modern action; the ushort tables are localized allText indices.
 */
unsigned char ClassicControlScheme[7] = {0, 1, 2, 3, 4, 5, 6};
unsigned char ModernControlScheme[7] = {0, 1, 3, 5, 4, 2, 6};
unsigned char ClassicControlSchemeForce[7] = {1, 0, 2, 5, 3, 4, 1};
unsigned char ModernControlSchemeForce[7] = {1, 0, 3, 2, 5, 4, 7};
uint16_t controlTextList[8] = {
    249, 250, 245, 299, 300, 301, 469, 251
};
uint16_t controlTextListForce[6] = {
    250, 249, 465, 466, 467, 468
};
uint32_t controlSubDraw[8] = {
    6, 2, 0x3f, 0, 400, 0x47, 0x10, 0x14
};

uint32_t languageMdef[25] = {
    0, 1, 0x3e, 500, 40, 6, 0,
    9, 0, 146, 50, 71,
    6, 2, 0x3f, 0, 80,
    8, UINT32_MAX, 145, 50,
    0x10, 0x45, 433, 0x14
};

uint32_t videoMdef[34] = {
    0, 3, 0x3e, 500, 40, 6, 0,
    9, 0, 463, 50, 72,
    9, 1, 372, 50, 73,
    8, 2, 450, 150,
    6, 2, 0x3f, 0, 80,
    8, UINT32_MAX, 462, 50,
    0x10, 0x45, 433, 0x14
};

/* Exact PDB-named audio option streams at RVAs 0x4C7280..0x4C7497. */
uint32_t audioMdef[44] = {
    0, 5, 0x3e, 500, 40, 6, 0,
    9, 0, 261, 50, 20,
    9, 1, 379, 50, 21,
    9, 2, 262, 50, 22,
    9, 3, 263, 50, 23,
    8, 4, 257, 103,
    6, 2, 0x3f, 0, 80,
    8, UINT32_MAX, 255, 50,
    0x10, 0x45, 433, 0x14
};

uint32_t audioMdef_Game[44] = {
    0, 5, 0x40, 500, 210, 6, 0,
    9, 0, 261, 50, 20,
    9, 1, 379, 50, 21,
    9, 2, 262, 50, 22,
    9, 3, 263, 50, 23,
    8, 4, 257, 103,
    6, 2, 0x40, 0, 290,
    8, UINT32_MAX, 255, 50,
    0x45, 220, 0x14, 0
};

uint32_t audioMusicMdef[46] = {
    0, 6, 3, 256, 40, 6, 2,
    9, 0, 76, 108, 38,
    8, 1, 72, 108,
    8, 2, 72, 109,
    8, 3, 75, 110,
    8, 4, 77, 111,
    8, 5, 78, 112,
    6, 2, 3, 256, 16,
    8, UINT32_MAX, 255, 50,
    0x10, 0x45, 433, 0x14, 0
};

uint32_t rusureQuitMenuMdef[25] = {
    0, 2, 3, 0, 100, 6, 2,
    8, 0, 272, 148,
    8, 1, 273, 147,
    0x3f, 0, 0,
    8, UINT32_MAX, 283, 50,
    0x45, 433, 0x14
};

/* Exact initialized PDB global at matched-PC RVA 0x4C7A40. */
uint32_t gameoverMdef[26] = {
    0, 1, 0x0f, 0, 2, 6, 0x40,
    0x40, 500, 290,
    8, UINT32_MAX, 375, 50,
    6, 0,
    0x3e, 500, 90,
    8, UINT32_MAX, 221, 50,
    0x45, 220,
    0x14
};

/* Exact four-word startup stream at matched-PC RVA 0x4C76F8. */
uint32_t startMdef[4] = {
    0, 0, 0x14, 0
};

/* Exact initialized PDB global creditsMdef at matched RVA 0x4C67E8. */
uint32_t creditsMdef[2] = {0x47, 0x14};

/* Exact initialized stream at matched-PC RVA 0x4C6CF0. The modifier item
 * points at modVars[8], whose source is LevelSelect and whose range is 1..14.
 */
uint32_t levelSelectMdef[15] = {
    UINT32_C(0x00000000), UINT32_C(0x00000001),
    UINT32_C(0x00000003), UINT32_C(0x00000000),
    UINT32_C(0x0000001E), UINT32_C(0x0000000C),
    UINT32_C(0x00000004), UINT32_C(0x00000006),
    UINT32_C(0x00000002), UINT32_C(0x00000009),
    UINT32_C(0x00000000), UINT32_C(0x00000002),
    UINT32_C(0x0000003D), UINT32_C(0x00000008),
    UINT32_C(0x00000014),
};

/* Exact PDB arrays at matched-PC RVAs 0x4C5D90 and 0x4C5DF0. */
uint32_t exitSelectMdef[23] = {
    UINT32_C(0x42), UINT32_C(6),
    UINT32_C(6), UINT32_C(0),
    UINT32_C(3), UINT32_C(100), UINT32_C(100),
    UINT32_C(8), UINT32_MAX, UINT32_C(239), UINT32_C(50),
    UINT32_C(0x42), UINT32_C(8),
    UINT32_C(6), UINT32_C(1),
    UINT32_C(3), UINT32_C(100), UINT32_C(100),
    UINT32_C(8), UINT32_MAX, UINT32_C(241), UINT32_C(50),
    UINT32_C(0x14)
};

uint32_t exitSelectMdef2[19] = {
    UINT32_C(6), UINT32_C(0),
    UINT32_C(3), UINT32_C(34), UINT32_C(200),
    UINT32_C(8), UINT32_MAX, UINT32_C(239), UINT32_C(50),
    UINT32_C(6), UINT32_C(1),
    UINT32_C(3), UINT32_C(470), UINT32_C(200),
    UINT32_C(8), UINT32_MAX, UINT32_C(241), UINT32_C(50),
    UINT32_C(0x14)
};

/*
 * Exact 75-entry command-width table at matched-PC RVA 0x4CAAE0.
 * A width includes the opcode itself.
 */
uint32_t mmsizes[75] = {
    2, 1, 1, 3, 3, 1, 2, 1, 4, 5,
    2, 2, 2, 2, 1, 3, 1, 1, 2, 2,
    1, 1, 2, 1, 1, 4, 2, 2, 2, 1,
    2, 3, 3, 1, 2, 2, 5, 6, 2, 2,
    2, 2, 0, 2, 0, 0, 2, 2, 1, 1,
    1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
    0, 1, 3, 3, 3, 3, 2, 2, 1, 2,
    2, 1, 2, 1, 2
};
