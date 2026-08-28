#ifndef JPB_SBASIC_H
#define JPB_SBASIC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct for_stack {
    float var;
    float target;
    float step;
    char *loc;
} for_stack;

typedef struct label {
    char name[10];
    char *p;
} label;

extern label label_table[256];
extern char tok;
extern char token_type;
extern char token[80];

void addline(int i, int lineno, int len, char *line);
void arith(char op, float *result, float *hold);
int assignment(void);
int bas_joypad(void);
int bas_keyheld(void);
int bas_keypress(void);
void basic_AddLine(char *line);
void basic_Init(void);
void basic_List(int from, int to);
void basic_LoadProgram(char *filename, int card);
void basic_New(void);
int basic_PrepareToRun(void);
int basic_Run(int start);
void basic_SaveProgram(char *filename, int card);
int consolesystem(void);
int exec_for(void);
int exec_gamecycle(void);
int exec_goto(void);
int exec_if(void);
int exec_rem(void);
int find_eol(void);
char *find_label(char *name);
float find_var(char *name);
int for_complete(for_stack *entry);
for_stack *fpop(void);
void fpush(for_stack *entry);
void get_exp(float *result);
int get_next_label(char *name);
int get_token(void);
int gosub(void);
char *gpop(void);
int gpush(char *position);
int greturn(void);
int input(void);
int isdelim(char value);
int iswhite(char value);
int label_init(void);
void level2(float *result);
void level3(float *result);
void level4(float *result);
void level5(float *result);
void level6(float *result);
int look_up(char *name);
int next(void);
void primitive(float *result);
int print(void);
void putback(void);
int robanim(void);
int robgraphic(int type);
int robnode(void);
int robprint(void);
int scan_labels(void);
void serror(int error);
void unary(char op, float *result);

#ifdef JPB_SBASIC_TESTING
void jpb_SbasicSetProgram(char *text);
char *jpb_SbasicProgramCursor(void);
float *jpb_SbasicVariables(void);
int jpb_SbasicDone(void);
int jpb_SbasicNeedsOutput(void);
int jpb_SbasicForDepth(void);
int jpb_SbasicGosubDepth(void);
#endif

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
static_assert(sizeof(for_stack) == 24, "for_stack layout changed");
static_assert(sizeof(label) == 24, "label layout changed");
#else
_Static_assert(sizeof(for_stack) == 24, "for_stack layout changed");
_Static_assert(sizeof(label) == 24, "label layout changed");
#endif

#endif
