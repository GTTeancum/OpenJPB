#ifndef JPB_MAIN_H
#define JPB_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

extern int debug_slomo;
extern int debug_singlestep;

int card_gInitVariables(void);
int console_CommandPause(
    int narg, char **arg_str, int *arg_int, float *arg_float);
int console_CommandStep(
    int narg, char **arg_str, int *arg_int, float *arg_float);
void initialize_main(void);

#ifdef __cplusplus
}
#endif

#endif
