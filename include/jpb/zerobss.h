#ifndef JPB_ZEROBSS_H
#define JPB_ZEROBSS_H

enum zerobssType {
    zerobssINT = 0,
    zerobssCHAR,
    zerobssSHORT,
    zerobssLONG,
    zerobssULONG,
    zerobssFLOAT,
    zerobssSVECTOR,
    zerobssSPRITEPtr,
    zerobssSOUNDHANDLE,
    zerobssCOLLIDE_INFO,
    zerobssplayerObject
};

enum zerobssVars {
    e_count_sf = 0,
    e_MODE_sf,
    e_arms_ld,
    e_count_ld,
    e_toss_ld,
    e_arms_td,
    e_count_td,
    e_shield_td,
    e_sptr_td,
    e_gDeathCount,
    e_start,
    e_succeed,
    e_failed,
    e_delay,
    e_stapsound,
    e_engaged_maul,
    e_timer_maul,
    e_keyh_kadu,
    e_speedh_kadu,
    e_slowh_kadu,
    e_lasth_kadu,
    e_knob,
    e_newcameraflag,
    e_streetsending,
    e_streetsendcampos,
    e_streetsendcamang,
    e_comboTally0,
    e_comboTally1,
    e_uberXRange,
    e_uberZRange,
    e_timesincetank,
    e_sptr_forceshield,
    e_zapman,
    e_glowtime,
    e_glowcolor,
    e_core_hack_ninehundred,
    e_bigwallcolor,
    e_currentmaze,
    e_trans,
    e_mazevel,
    e_planktimer,
    e_coretimers,
    e_coreplotted,
    e_corecol,
    e_score1,
    e_score2,
    e_pilotsKilled,
    e_vscroll,
    e_bestinfo,
    e_timeoffscreen,
    e_offscreenbleeps,
    e_tankspeed,
    e_tanktarget0,
    e_tanktarget1,
    e_elapsed,
    e_fireSequence,
    e_fireElapsed,
    e_tankwhich,
    e_stapangle,
    e_stapshot,
    e_totalframes,
    e_playertankindex,
    e_tankdrivers,
    e_stapbikeindex,
    e_tanknoise,
    e_turretnoise,
    NUM_VARS
};

void *ZeroBSS(zerobssVars var, zerobssType type, int size);
void ZeroBSS_ClearAll(void);

static_assert(zerobssplayerObject == 10, "zerobssType values changed");
static_assert(NUM_VARS == 66, "zerobssVars values changed");

#endif
