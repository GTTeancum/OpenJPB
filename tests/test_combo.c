#include "jpb/combo.h"
#include "jpb/anim.h"
#include "jpb/game.h"
#include "jpb/scene.h"
#include "jpb/world.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf( \
                stderr, \
                "FAIL %s:%d: %s\n", \
                __FILE__, \
                __LINE__, \
                #condition); \
            ++failures; \
        } \
    } while (0)

static void test_combo_availability_and_timing(void)
{
    playerObject player;
    Combo combo;
    int32_t cpad[2] = {0, 0};

    memset(&player, 0, sizeof(player));
    memset(&combo, 0, sizeof(combo));
    memset(&GameStruct, 0, sizeof(GameStruct));
    player.paCombos = &combo;
    player.maxCombos = 1;
    player.playerID = 0;
    player.playernum = 0;
    combo.Len = 1;
    combo.Index = UINT8_C(0xff);
    strcpy(combo.String, "n");

    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    CHECK(strcmp(player.PreMotion, "n") == 0);

    GameStruct.jediComboMask[0].m[0] = 1;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);
    CHECK(strcmp(player.PreMotion, "n.") == 0);

    memset(&GameStruct, 0, sizeof(GameStruct));
    LevelSelect = 11;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);
    LevelSelect = 13;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);
    LevelSelect = 0;
    GameStruct.versusModeFlag = 1;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);
    GameStruct.versusModeFlag = 0;
    player.playerID = 9;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);

    player.playerID = 0;
    GameStruct.jediComboMask[0].m[0] = 1;
    combo.kdmin = 3;
    combo.kdmax = 5;
    player.vtime = 0;
    player.ctime = 2 * 0x200;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    CHECK(strcmp(player.PreMotion, "nJ") == 0);
    CHECK(player.mtime == player.ctime);

    player.mtime = 0;
    player.vtime = 0;
    player.ctime = 6 * 0x200;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    CHECK(strcmp(player.PreMotion, "nK") == 0);
    CHECK(player.mtime == 0);

    combo.kdmin = 0;
    combo.kdmax = 0;
    combo.comboFlags = UINT32_C(1);
    player.heldMask = 0;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    player.heldMask = 1;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 1);

    combo.comboFlags = UINT32_C(0x01000000);
    combo.userData = 7;
    player.currentMotion = 6;
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    CHECK(strcmp(player.PreMotion, "K") == 0);

    combo.comboFlags = UINT32_C(0x00200000);
    player.pFlags = UINT32_C(0x00200000);
    strcpy(player.PreMotion, "n");
    CHECK(combo_CheckCombo(cpad, &player) == 0);
    CHECK((player.pFlags & UINT32_C(0x01000000)) != 0);
}

static void test_combo_reset_masks(void)
{
    playerObject player;
    int key;

    memset(&player, 0, sizeof(player));
    player.ctime = 1;
    player.vtime = 2;
    player.dtime = 3;
    player.mtime = 4;
    player.pFlags = UINT32_C(0x00220000);
    player.playerPad.bufferedbits = UINT32_MAX;
    player.chainSlack = 12;
    player.chainSlackEnd = 34;
    player.heldMask = UINT32_C(0xffff);
    player.releaseMask = UINT32_C(0xffff);
    strcpy(player.PreMotion, "n.s.w");
    player.bheld[0] = 1;
    player.bheld[15] = 2;

    combo_ResetComboEngine(456, &player);
    CHECK(player.ctime == 456);
    CHECK(player.vtime == 456);
    CHECK(player.dtime == 456);
    CHECK(player.mtime == 0);
    CHECK(player.PreMotion[0] == '\0');
    CHECK(player.playerPad.bufferedbits == 0);
    CHECK(player.chainSlack == 0);
    CHECK(player.chainSlackEnd == 0);
    CHECK((player.pFlags & UINT32_C(0x00020000)) == 0);
    CHECK((player.pFlags & UINT32_C(0x01200000)) ==
          UINT32_C(0x01200000));
    CHECK(player.heldMask == UINT32_C(0x8001));
    CHECK(player.releaseMask == 0);
    for (key = 1; key < 15; ++key) {
        CHECK(player.bheld[key] == 0);
    }
}

static void test_combo_award_eligibility(void)
{
    Combo combos[JPB_COMBO_AWARD_THRESHOLD_CAPACITY][2];
    Combo *saved[JPB_COMBO_AWARD_THRESHOLD_CAPACITY];
    int32_t saved_thresholds[JPB_COMBO_AWARD_THRESHOLD_CAPACITY];
    int jedi;

    memset(combos, 0, sizeof(combos));
    for (jedi = 0;
         jedi < JPB_COMBO_AWARD_THRESHOLD_CAPACITY;
         ++jedi) {
        saved[jedi] = gaPlayerData[jedi].paCombos;
        saved_thresholds[jedi] =
            jpb_comboAwardHitThreshold[jedi];
        gaPlayerData[jedi].paCombos = combos[jedi];
        jpb_comboAwardHitThreshold[jedi] = jedi + 2;
        combos[jedi][0].numHits = (int16_t)(jedi + 2);
        combos[jedi][1].numHits = (int16_t)(jedi + 3);
        CHECK(combo_ValidComboAward(jedi, 0) == 1);
        CHECK(combo_ValidComboAward(jedi, 1) == 0);
    }
    for (jedi = 0;
         jedi < JPB_COMBO_AWARD_THRESHOLD_CAPACITY;
         ++jedi) {
        gaPlayerData[jedi].paCombos = saved[jedi];
        jpb_comboAwardHitThreshold[jedi] =
            saved_thresholds[jedi];
    }
}

static void check_combo_award_threshold(
    int player_id,
    const char *const *strings,
    int combo_count,
    int expected_threshold)
{
    playerObject player;
    Combo combos[6];
    int index;

    memset(&player, 0, sizeof(player));
    memset(combos, 0, sizeof(combos));
    memset(&GameStruct, 0, sizeof(GameStruct));
    player.playerID = (int16_t)player_id;
    player.playernum = 0;
    player.paCombos = combos;
    player.maxCombos = (int16_t)combo_count;
    GameStruct.jediComboMask[player_id].m[0] = UINT32_MAX;
    jpb_comboAwardHitThreshold[0] = -1;
    for (index = 0; index < combo_count; ++index) {
        strcpy(combos[index].String, strings[index]);
    }

    combo_InitComboData(&player);
    CHECK(jpb_comboAwardHitThreshold[0] ==
          expected_threshold);
}

static void test_combo_award_thresholds(void)
{
    static const char *const three_hit[] = {
        "nnn", "nns", "nnw", "nsn", "nss", "nsw"
    };
    static const char *const four_hit[] = {
        "nnnn", "nnns", "nnnw"
    };

    check_combo_award_threshold(0, four_hit, 2, 4);
    check_combo_award_threshold(0, four_hit, 3, 6);
    check_combo_award_threshold(5, four_hit, 3, 6);
    check_combo_award_threshold(1, three_hit, 3, 3);
    check_combo_award_threshold(1, three_hit, 4, 6);
    check_combo_award_threshold(4, three_hit, 4, 6);
    check_combo_award_threshold(8, three_hit, 4, 6);
    check_combo_award_threshold(2, three_hit, 2, 3);
    check_combo_award_threshold(2, three_hit, 3, 6);
    check_combo_award_threshold(3, three_hit, 5, 3);
    check_combo_award_threshold(3, three_hit, 6, 6);
    check_combo_award_threshold(6, three_hit, 6, 0);
}

int main(void)
{
    playerObject player;
    sceneObject scene;
    animObject animation;
    int32_t cpad[2] = {0, 1};

    memset(&player, 0, sizeof(player));
    memset(&scene, 0, sizeof(scene));
    memset(&animation, 0, sizeof(animation));
    player.playerRoot.pParent = &scene.sceneRoot;
    scene.pAnim = &animation.animRoot;
    totalframes = 100;
    combo_CheckHeldPad(cpad, &player, 1, 3);
    CHECK(player.bheld[0] == 100);
    CHECK(player.heldMask == 0);

    totalframes = 1637;
    combo_CheckHeldPad(cpad, &player, 1, 3);
    CHECK(player.heldMask == 1);

    cpad[1] = 0;
    combo_CheckHeldPad(cpad, &player, 1, 3);
    CHECK(player.bheld[0] == 0);
    CHECK(player.releaseMask == 1);
    combo_CheckHeldPad(cpad, &player, 1, 3);
    CHECK(player.releaseMask == 0);

    memset(&player, 0, sizeof(player));
    player.playerRoot.pParent = &scene.sceneRoot;
    memset(&OptionStruct, 0, sizeof(OptionStruct));
    animation.animFrameIndex = 0;
    totalframes = 100;
    cpad[0] = 0x10;
    cpad[1] = 0x20;
    combo_ReadCombo(cpad, &player);
    CHECK(strcmp(player.PreMotion, "n") == 0);
    CHECK(strcmp(player.HeldMotion, "e") == 0);
    CHECK(player.ctime == 100);
    CHECK(player.dtime == 100);

    player.playerPad.bufferedbits = 0x1234;
    totalframes = 3000;
    cpad[0] = 0;
    cpad[1] = 0;
    combo_ReadCombo(cpad, &player);
    CHECK(player.playerPad.bufferedbits == 0);
    CHECK(player.ctime == 100);
    CHECK(player.HeldMotion[0] == '\0');

    totalframes = 13000;
    combo_ReadCombo(cpad, &player);
    CHECK(player.ctime == 13000);
    CHECK(player.vtime == 13000);
    CHECK(player.dtime == 13000);
    CHECK(player.PreMotion[0] == '\0');

    memset(&player, 0, sizeof(player));
    player.playerRoot.pParent = &scene.sceneRoot;
    animation.animFrameIndex = 2 << 12;
    player.pFlags = UINT32_C(0x00020000);
    player.ctime = 25;
    player.chainSlack = 0;
    player.chainSlackEnd = 7;
    player.playerPad.bufferedbits = 0xffff;
    strcpy(player.PreMotion, "n");
    cpad[0] = 1;
    cpad[1] = 0;
    combo_ReadCombo(cpad, &player);
    CHECK(strcmp(player.PreMotion, "nSERR") == 0);
    CHECK((player.pFlags & UINT32_C(0x00020000)) == 0);
    CHECK(player.chainSlack == 0);
    CHECK(player.chainSlackEnd == 0);
    CHECK(player.ctime == 25 + 0x200);
    CHECK(player.playerPad.bufferedbits == 0);

    memset(&player, 0, sizeof(player));
    player.playerRoot.pParent = &scene.sceneRoot;
    animation.animFrameIndex = 0;
    player.pFlags = UINT32_C(0x02000000);
    cpad[0] = 0x4000;
    cpad[1] = 0x80;
    totalframes = 14000;
    combo_ReadCombo(cpad, &player);
    CHECK(cpad[0] == 0);
    CHECK(cpad[1] == 0);

    {
        Combo combos[1];

        memset(&player, 0, sizeof(player));
        memset(combos, 0, sizeof(combos));
        player.playerRoot.pParent = &scene.sceneRoot;
        player.paCombos = combos;
        player.maxCombos = 1;
        combos[0].Len = 1;
        combos[0].Index = UINT8_C(0xff);
        strcpy(combos[0].String, "n");
        strcpy(player.PreMotion, "n");
        memset(&GameStruct, 0, sizeof(GameStruct));
        GameStruct.jediComboMask[0].m[0] = 1;
        CHECK(combo_CheckCombo(cpad, &player) == 1);
        CHECK(strcmp(player.PreMotion, "n.") == 0);
        CHECK((player.pFlags &
               UINT32_C(0x00200000)) != 0);

        memset(&player, 0, sizeof(player));
        player.playerRoot.pParent = &scene.sceneRoot;
        player.paCombos = combos;
        player.maxCombos = 1;
        combos[0].comboFlags =
            UINT32_C(0x00200000);
        strcpy(combos[0].String, "n+e");
        strcpy(player.PreMotion, "w+n+e");
        animation.Lock = 0;
        CHECK(combo_CheckPreCombo(cpad, &player) == 1);
        CHECK(strcmp(player.PreMotion, "n+e") == 0);

        player.maxCombos = 0;
        strcpy(
            player.PreMotion,
            "1234567890123456789012345678901");
        CHECK(combo_CheckCombo(cpad, &player) == 0);
        CHECK(strcmp(
                  player.PreMotion,
                  "890123456789012345678901") == 0);
    }

    {
        Combo combos[4];

        memset(&player, 0, sizeof(player));
        memset(combos, 0, sizeof(combos));
        memset(&GameStruct, 0, sizeof(GameStruct));
        memset(
            jpb_comboAwardHitThreshold,
            0xff,
            sizeof(jpb_comboAwardHitThreshold));
        player.paCombos = combos;
        player.maxCombos = 4;
        player.playerID = 0;
        player.playernum = 0;
        combos[0].Index = 47;
        combos[0].comboFlags = UINT32_C(0x04000000);
        strcpy(combos[0].String, "n");
        combos[1].Index = 48;
        combos[1].comboFlags = UINT32_C(0x04000000);
        strcpy(combos[1].String, "s");
        combos[2].Index = 46;
        combos[2].comboFlags = UINT32_C(0x04000000);
        strcpy(combos[2].String, "w");
        combos[3].Index = 20;
        strcpy(combos[3].String, "nswf");

        combo_InitComboData(&player);
        CHECK(combos[0].Len == 1);
        CHECK(combos[0].Index == 106);
        CHECK(combos[0].numHits == 1);
        CHECK(combos[1].Index == 107);
        CHECK(combos[1].numHits == 1);
        CHECK(combos[2].Index == 105);
        CHECK(combos[2].numHits == 1);
        CHECK(combos[3].Len == 4);
        CHECK(combos[3].numHits == 6);
        CHECK(jpb_comboAwardHitThreshold[0] == 4);
    }

    memset(&player, 0, sizeof(player));
    gGlobalTimer = 1000;
    player.bheld[5] = 400;
    CHECK(combo_GetHeldTime(&player, 0x20) == 600);
    CHECK(combo_GetHeldTime(&player, 0) == 0);

    test_combo_availability_and_timing();
    test_combo_reset_masks();
    test_combo_award_eligibility();
    test_combo_award_thresholds();

    if (failures != 0) {
        fprintf(
            stderr,
            "%d combo test(s) failed\n",
            failures);
        return 1;
    }
    puts("combo tests passed");
    return 0;
}
