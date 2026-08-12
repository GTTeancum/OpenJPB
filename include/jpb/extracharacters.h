#ifndef JPB_EXTRACHARACTERS_H
#define JPB_EXTRACHARACTERS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Exact matched-PC PDB enum 0x104E. Aliases are retained because they carry
 * useful source-level meaning even when they share the same numeric value.
 */
typedef enum model_id {
    obi_wan_model = 0,
    qui_gon_model = 1,
    mace_model = 2,
    adi_model = 3,
    plo_model = 4,
    maul_p_model = 5,
    amidala_model = 6,
    panaka_model = 7,
    ki_adi_model = 8,
    last_jedi_model = 9,
    maul_model = 9,
    jar_jar_model = 10,
    queen_model = 11,
    protocol_model = 12,
    r2_model = 13,
    worker_model = 14,
    pilot_model = 15,
    handmaid_model = 16,
    battle_d_model = 17,
    rifle_model = 18,
    grapple_model = 19,
    grap_up_model = 20,
    flame_model = 21,
    plasma_model = 22,
    stap_model = 23,
    bd_stap_model = 24,
    bd_kadu_model = 25,
    destroye_model = 26,
    destroym_model = 27,
    destroyu_model = 28,
    goliath_model = 29,
    loader_model = 30,
    mtt_model = 31,
    plant_2_model = 32,
    kadu_model = 33,
    gunchief_model = 34,
    aat_model = 35,
    tusken_s_model = 36,
    tusken_r_model = 37,
    probe_model = 38,
    probe_b_model = 39,
    desert_b_model = 40,
    turret_d_model = 41,
    turret_u_model = 42,
    maul_d_model = 43,
    fulump_model = 44,
    nuna_model = 45,
    pikobis_model = 46,
    droid_f_model = 47,
    thug_1_model = 48,
    thug_2_model = 49,
    thug_3_model = 50,
    thug_4_model = 51,
    peck_model = 52,
    gungan_1_model = 53,
    gungan_2_model = 54,
    gungan_s_model = 55,
    gungan_b_model = 56,
    jawa_model = 57,
    anakin_model = 58,
    n_pilot_model = 59,
    n_guard_model = 60,
    commandr_model = 61,
    security_model = 62,
    peko_model = 63,
    hombre_1_model = 64,
    chica_1_model = 65,
    bossnass_model = 66,
    taxi_1_model = 67,
    taxi_2_model = 68,
    taxi_3_model = 69,
    bus_model = 70,
    hype_model = 71,
    beacon_model = 72,
    sarlacc_model = 73,
    turret_s_model = 74,
    boulder_model = 75,
    blades_model = 76,
    worm_model = 77,
    roach_model = 78,
    jar_jar_playable_model = 79,
    LAST_PLAYABLE_MODEL = 80,
    obi_wan1_model = 80,
    qui_gon1_model = 81,
    mace1_model = 82,
    adi1_model = 83,
    plo1_model = 84,
    FIRST_LEVEL_MODEL = 85,
    cannon_model = 85,
    fed_door_model = 86,
    piston_model = 87,
    fan_model = 88,
    spore_model = 89,
    mushroom_model = 90,
    tree_model = 91,
    spire_model = 92,
    box_model = 93,
    lift_1_model = 94,
    lift_2_model = 95,
    lift_3_model = 96,
    coffin_model = 97,
    s_door_model = 98,
    twister_model = 99,
    pal_door_model = 100,
    laser_h_model = 101,
    laser_v_model = 102,
    cor_lift_model = 103,
    cor_fan_model = 104,
    han_door_model = 105,
    turret_model = 106,
    rubble_d_model = 107,
    laser_s_model = 108,
    conc_model = 109,
    th_gate_model = 110,
    rubble_r_model = 111,
    qn_ship_model = 112,
    sithbike_model = 113,
    hngdr_model = 114,
    last_model = 115
} model_id;

/* Exact matched-PC PDB type 0x6C4D. */
typedef struct ExtraCharacter {
    const model_id ID;
    const int TextIndex;
    const int CanReflect;
    const int CanForcePower;
    const int CanLedgeClimb;
    int Unlocked;
} ExtraCharacter;

enum {
    JPB_EXTRA_CHARACTER_COUNT = 14
};

extern ExtraCharacter ExtraCharacters[JPB_EXTRA_CHARACTER_COUNT];
extern const size_t ExtraCharactersSize;

ExtraCharacter *GetCharacterByID(model_id ID);
int IsExtraCharacter(model_id ID);
int extracharacter_CanForcePower(model_id ID);
int extracharacter_CanLedgeClimb(model_id ID);
int extracharacter_CanReflect(model_id ID);

#if defined(__cplusplus)
static_assert(sizeof(model_id) == 4, "model_id must match PDB enum width");
static_assert(
    sizeof(ExtraCharacter) == 24,
    "ExtraCharacter must match PDB size");
#else
_Static_assert(sizeof(model_id) == 4, "model_id must match PDB enum width");
_Static_assert(
    sizeof(ExtraCharacter) == 24,
    "ExtraCharacter must match PDB size");
#endif

#ifdef __cplusplus
}
#endif

#endif
