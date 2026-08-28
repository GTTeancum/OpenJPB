/*
 * Complete reviewed reconstruction of Jedi Power BASIC.
 *
 * PDB module 0075 fixes the 55 procedure boundaries, private layouts, and
 * globals. Bodies and initialized data were recovered from game.exe RVAs
 * 0xF0B40 through 0xF4EA5. Shipped parser defects and undefined non-void
 * returns are retained.
 */

#include "jpb/sbasic.h"

#include "jpb/anim.h"
#include "jpb/colorb.h"
#include "jpb/console.h"
#include "jpb/debugtext.h"
#include "jpb/input.h"
#include "jpb/model.h"
#include "jpb/whook.h"
#include "jpb/win_memcard.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SBASIC_LINES = 256,
    SBASIC_VARIABLES = 26,
    SBASIC_STACK_DEPTH = 25,
    SBASIC_TOKEN_BYTES = 80,
    SBASIC_COMMAND_BYTES = 20,
    SBASIC_LABEL_NAME_BYTES = 10
};

enum sbasic_token_kind {
    TOKEN_DELIMITER = 1,
    TOKEN_VARIABLE = 2,
    TOKEN_NUMBER = 3,
    TOKEN_COMMAND = 4,
    TOKEN_IDENTIFIER = 5,
    TOKEN_QUOTED = 6
};

enum sbasic_command_token {
    TOK_PRINT = 1, TOK_INPUT, TOK_IF, TOK_THEN, TOK_FOR, TOK_NEXT, TOK_TO,
    TOK_GOTO, TOK_EOL, TOK_FINISHED, TOK_GOSUB, TOK_RETURN, TOK_END,
    TOK_ELSE, TOK_STEP, TOK_REM, TOK_CYCLE, TOK_SYSTEM, TOK_ANIM, TOK_PLAY,
    TOK_NODE, TOK_LINE, TOK_POINT, TOK_CIRCLE, TOK_MOVE, TOK_SPRINT,
    TOK_KEYHELD, TOK_KEYPRESS, TOK_JOYPAD
};

typedef struct _basic_line {
    int line_number;
    char *line;
} _basic_line;

typedef struct commands {
    char command[SBASIC_COMMAND_BYTES];
    char tok;
} commands;

static char *basic_program;
static char *prog;
static int done;
static int needoutput;
static int ftos;
static int gtos;
static _basic_line basic_line[SBASIC_LINES];
static float variables[SBASIC_VARIABLES];
static char *line;
static for_stack fstack[SBASIC_STACK_DEPTH];
static char *gstack[SBASIC_STACK_DEPTH];

label label_table[SBASIC_LINES];
char tok;
char token_type;
char token[SBASIC_TOKEN_BYTES];

static const commands table[] = {
    {"print", TOK_PRINT}, {"input", TOK_INPUT}, {"if", TOK_IF},
    {"then", TOK_THEN}, {"else", TOK_ELSE}, {"goto", TOK_GOTO},
    {"for", TOK_FOR}, {"next", TOK_NEXT}, {"to", TOK_TO},
    {"gosub", TOK_GOSUB}, {"return", TOK_RETURN}, {"end", TOK_END},
    {"step", TOK_STEP}, {"rem", TOK_REM}, {"cycle", TOK_CYCLE},
    {"system", TOK_SYSTEM}, {"anim", TOK_ANIM}, {"play", TOK_PLAY},
    {"node", TOK_NODE}, {"line", TOK_LINE}, {"point", TOK_POINT},
    {"circle", TOK_CIRCLE}, {"move", TOK_MOVE}, {"sprint", TOK_SPRINT},
    {"keyheld", TOK_KEYHELD}, {"keypress", TOK_KEYPRESS},
    {"joypad", TOK_JOYPAD}, {"", TOK_END}
};

static char *e[] = {
    "Syntax error", "Unbalanced parens", "No expression present",
    "Expected = ; or somesuch", "Not a variable", "Lable table full",
    "Duplicate label", "Undefined label", "Expected THEN", "Expected TO",
    "Too many nested FOR loops", "NEXT without FOR",
    "Too many nested GOSUBs", "RETURN without GOSUB", "Unexpected STEP"
};

static int exec_command(void);

static int variable_index(char value)
{
    return toupper((unsigned char)value) - 'A';
}

static void putback_token(void)
{
    char *scan = token;
    while (*scan++ != '\0') {
        --prog;
    }
}

static void parse_expression_argument(float *value)
{
    get_token();
    if (token[0] == '\0') {
        serror(2);
        return;
    }
    level2(value);
    putback_token();
}

static void make_program_filename(char output[64], char *filename)
{
    int i = 0;
    while (*filename != '\0' && *filename != '.' && i < 8) {
        output[i++] = (char)toupper((unsigned char)*filename++);
    }
    memcpy(output + i, ".JPB", 5);
}

/* 0xF0B40, 97 bytes. */
void addline(int i, int lineno, int len, char *source)
{
    basic_line[i].line_number = lineno;
    basic_line[i].line = (char *)malloc((size_t)len);
    if (basic_line[i].line != NULL) {
        strcpy(basic_line[i].line, source);
    }
}

/* 0xF0BB0, 274 bytes. */
void arith(char op, float *result, float *hold)
{
    float base;
    float exponent;
    switch (op) {
    case '-': *result -= *hold; return;
    case '+': *result += *hold; return;
    case '*': *result *= *hold; return;
    case '/': *result /= *hold; return;
    case '%': *result = 0.0f; return;
    case '^':
        base = *result;
        if (*hold == 0.0f) {
            *result = 1.0f;
            return;
        }
        exponent = *hold - 1.0f;
        while (exponent > 0.0f) {
            exponent -= 1.0f;
            *result *= base;
        }
        return;
    default: return;
    }
}

/* 0xF0CD0, 151 bytes. */
int assignment(void)
{
    float value;
    int index;
    get_token();
    if (done != 0) return 0;
    if (!isalpha((unsigned char)token[0])) {
        serror(4);
        return -1;
    }
    index = variable_index(token[0]);
    get_token();
    if (done != 0) return 0;
    if (token[0] != '=') {
        serror(3);
        return 0;
    }
    get_exp(&value);
    variables[index] = value;
    return 0;
}

/* 0xF0D70, 146 bytes. */
int bas_joypad(void)
{
    float result;
    parse_expression_argument(&result);
    if (done != 0) return 0;
    variables['Z' - 'A'] =
        padbuttonpressed(0, (int)result) != 0 ? 1.0f : 0.0f;
    return 1;
}

/* 0xF0E10, 181 bytes. */
int bas_keyheld(void)
{
    float result;
    int pressed;
    get_token();
    if (done != 0) return 0;
    if (token_type == TOKEN_QUOTED) {
        pressed = KeyHeld((unsigned char)token[0]);
        variables['Z' - 'A'] = (float)pressed;
        return pressed;
    }
    putback_token();
    parse_expression_argument(&result);
    if (done != 0) return 0;
    variables['Z' - 'A'] = KeyHeld((int)result) != 0 ? 1.0f : 0.0f;
    return 1;
}

/* 0xF0ED0, 181 bytes. */
int bas_keypress(void)
{
    float result;
    int pressed;
    get_token();
    if (done != 0) return 0;
    if (token_type == TOKEN_QUOTED) {
        pressed = KeyPressed((unsigned char)token[0]);
        variables['Z' - 'A'] = (float)pressed;
        return pressed;
    }
    putback_token();
    parse_expression_argument(&result);
    if (done != 0) return 0;
    variables['Z' - 'A'] = KeyPressed((int)result) != 0 ? 1.0f : 0.0f;
    return 1;
}

/* 0xF0F90, 433 bytes. */
void basic_AddLine(char *source)
{
    char *content;
    int lineno = atoi(source);
    int len = (int)strlen(source) + 1;
    int i = 0;
    if (len > 255) {
        console_AddString("Line too long\n");
        return;
    }
    content = source;
    while (isdigit((unsigned char)*content)) ++content;
    while (*content == ' ' || *content == '\t') ++content;
    while (i < SBASIC_LINES && basic_line[i].line != NULL) {
        if (lineno <= basic_line[i].line_number) break;
        ++i;
    }
    if (i >= SBASIC_LINES) return;
    if (basic_line[i].line == NULL) {
        if (*content != '\0') addline(i, lineno, len, source);
        return;
    }
    if (lineno == basic_line[i].line_number) {
        free(basic_line[i].line);
        basic_line[i].line = NULL;
        if (*content == '\0') return;
    } else {
        if (*content == '\0') return;
        if (basic_line[SBASIC_LINES - 1].line != NULL) {
            free(basic_line[SBASIC_LINES - 1].line);
        }
        if (i < SBASIC_LINES - 1) {
            memmove(&basic_line[i + 1], &basic_line[i],
                (size_t)(SBASIC_LINES - 1 - i) * sizeof(basic_line[0]));
        }
    }
    addline(i, lineno, len, source);
}

/* 0xF1150, 20 bytes. */
void basic_Init(void)
{
    memset(basic_line, 0, sizeof(basic_line));
}

/* 0xF1170, 98 bytes. */
void basic_List(int from, int to)
{
    int i;
    for (i = 0; i < SBASIC_LINES; ++i) {
        if (basic_line[i].line != NULL &&
            from <= basic_line[i].line_number &&
            basic_line[i].line_number < to) {
            console_Printf("%s\n", basic_line[i].line);
        }
    }
}

/* 0xF11E0, 365 bytes. */
void basic_LoadProgram(char *filename, int card)
{
    char tempname[64];
    unsigned char *text;
    unsigned long size;
    char *position;
    int error;
    make_program_filename(tempname, filename);
    memcard_on();
    error = memcard_LoadFile(card, tempname, &text, &size);
    memcard_off();
    if (error != 0) return;
    basic_New();
    position = (char *)text;
    for (; size > 0; --size) {
        char *end = position;
        while (*end != '\0' && *end != '\n') {
            ++end;
            --size;
        }
        *end = '\0';
        if (atoi(position) != 0) basic_AddLine(position);
        position = end + 1;
    }
    free(text);
}

/* 0xF1350, 98 bytes. */
void basic_New(void)
{
    int i;
    for (i = 0; i < SBASIC_LINES; ++i) {
        if (basic_line[i].line != NULL) {
            free(basic_line[i].line);
            basic_line[i].line = NULL;
        }
    }
    if (basic_program != NULL) {
        free(basic_program);
        basic_program = NULL;
    }
}

/* 0xF13C0, 756 bytes. */
int basic_PrepareToRun(void)
{
    int total = 0;
    int i;
    if (basic_program != NULL) {
        free(basic_program);
        basic_program = NULL;
    }
    for (i = 0; i < SBASIC_LINES; ++i) {
        if (basic_line[i].line != NULL) {
            total += (int)strlen(basic_line[i].line) + 1;
        }
    }
    ++total;
    if (total != 0) {
        basic_program = (char *)malloc((size_t)total);
        if (basic_program != NULL) {
            basic_program[0] = '\0';
            for (i = 0; i < SBASIC_LINES; ++i) {
                if (basic_line[i].line != NULL) {
                    strcat(basic_program, basic_line[i].line);
                    strcat(basic_program, "\n");
                }
            }
            done = 0;
            prog = basic_program;
            scan_labels();
            prog = basic_program;
            ftos = 0;
            gtos = 0;
            done = 0;
            needoutput = 0;
        }
    }
}

/* 0xF16C0, 122 bytes. */
int basic_Run(int start)
{
    (void)start;
    needoutput = 0;
    for (;;) {
        line = prog;
        token_type = (char)get_token();
        if (done != 0) break;
        if (needoutput != 0) return 1;
        exec_command();
        if (done != 0) break;
        if (needoutput != 0) return 1;
    }
    free(basic_program);
    basic_program = NULL;
    return 0;
}

/* 0xF1740, 424 bytes. */
void basic_SaveProgram(char *filename, int card)
{
    char tempname[64];
    int total = 0;
    int i;
    if (basic_program != NULL) {
        free(basic_program);
        basic_program = NULL;
    }
    for (i = SBASIC_LINES - 1; i >= 0; --i) {
        if (basic_line[i].line != NULL) {
            total += (int)strlen(basic_line[i].line) + 1;
        }
    }
    ++total;
    if (total == 0) return;
    basic_program = (char *)malloc((size_t)total);
    if (basic_program == NULL) return;
    basic_program[0] = '\0';
    for (i = 0; i < SBASIC_LINES && basic_line[i].line != NULL; ++i) {
        strcat(basic_program, basic_line[i].line);
        strcat(basic_program, "\n");
    }
    make_program_filename(tempname, filename);
    memcard_on();
    memcard_SaveFile(card, tempname, (unsigned char *)basic_program,
        (unsigned long)total);
    console_Printf("Saved %d bytes to %s on card %d\n", total, tempname, card);
    memcard_off();
}

/* 0xF18F0, 419 bytes. */
int consolesystem(void)
{
    char output[256] = "";
    char formatted[24];
    float answer;

    get_token();
    for (;;) {
        if (done != 0) return 0;
        if (token_type == TOKEN_QUOTED) {
            strncat(output, token, 256);
            get_token();
            if (done != 0) return 0;
        } else {
            putback_token();
            get_token();
            if (token[0] == '\0') {
                serror(2);
            } else {
                level2(&answer);
                putback_token();
            }
            get_token();
            if (done != 0) return 0;
            sprintf(formatted, "%f", answer);
            strncat(output, formatted, 256);
        }
        if (token[0] == ',') strncat(output, " ", 256);
        if (token[0] != ';' && token[0] != ',') {
            console_ProcessCommandLine(output);
            return 0;
        }
        get_token();
    }
}

/* 0xF1AA0, 3964 bytes, local. */
static int exec_command(void)
{
    if (token_type == TOKEN_VARIABLE) {
        putback();
        assignment();
        return 0;
    }

    switch ((unsigned char)tok) {
    case TOK_PRINT:
        print();
        break;
    case TOK_INPUT:
        input();
        break;
    case TOK_IF:
        exec_if();
        break;
    case TOK_THEN:
    case TOK_TO:
    case TOK_EOL:
    case TOK_ELSE:
    case TOK_STEP:
    case TOK_PLAY:
        break;
    case TOK_FOR:
        exec_for();
        break;
    case TOK_NEXT:
        next();
        break;
    case TOK_GOTO:
        exec_goto();
        break;
    case TOK_FINISHED:
    case TOK_END:
        done = 1;
        break;
    case TOK_GOSUB:
        gosub();
        break;
    case TOK_RETURN:
        greturn();
        break;
    case TOK_REM:
        exec_rem();
        break;
    case TOK_CYCLE:
        exec_gamecycle();
        break;
    case TOK_SYSTEM:
        consolesystem();
        break;
    case TOK_ANIM:
        robanim();
        break;
    case TOK_NODE:
        robnode();
        break;
    case TOK_LINE:
    case TOK_POINT:
    case TOK_CIRCLE:
    case TOK_MOVE:
        robgraphic((unsigned char)tok);
        break;
    case TOK_SPRINT:
        robprint();
        break;
    case TOK_KEYHELD:
        bas_keyheld();
        break;
    case TOK_KEYPRESS:
        bas_keypress();
        break;
    case TOK_JOYPAD:
        bas_joypad();
        break;
    default:
        break;
    }
    return 0;
}

/* 0xF2A20, 510 bytes. */
int exec_for(void)
{
    for_stack entry;
    float initial;

    get_token();
    if (done != 0) return 0;
    if (!isalpha((unsigned char)token[0])) {
        serror(4);
        return 0;
    }
    entry.var = (float)variable_index(token[0]);
    get_token();
    if (done != 0) return 0;
    if (token[0] != '=') {
        serror(3);
        return 0;
    }
    get_exp(&initial);
    variables[(int)entry.var] = initial;
    get_token();
    if (done != 0) return 0;
    if (tok != TOK_TO) {
        serror(9);
        return 0;
    }
    get_exp(&entry.target);
    entry.step = 1.0f;
    get_token();
    if (done != 0) return 0;
    if (tok == TOK_STEP) {
        get_exp(&entry.step);
    } else {
        putback();
    }
    if (for_complete(&entry)) {
        if (tok == TOK_NEXT) return 0;
        do {
            get_token();
            if (done != 0) return 0;
        } while (tok != TOK_NEXT);
        return 0;
    }
    entry.loc = prog;
    fpush(&entry);
    return 0;
}

/* 0xF2C20, 11 bytes. */
int exec_gamecycle(void)
{
    needoutput = 1;
}

/* 0xF2C30, 186 bytes. */
int exec_goto(void)
{
    char *position;
    get_token();
    if (done != 0) return 0;
    position = find_label(token);
    if (position == NULL) {
        serror(7);
    } else {
        prog = position;
    }
    return 0;
}

/* 0xF2CF0, 501 bytes. */
int exec_if(void)
{
    float left;
    float right;
    char relation;
    int condition = 0;

    get_token();
    if (token[0] == '\0') {
        serror(2);
    } else {
        level2(&left);
        putback_token();
    }
    get_token();
    if (done != 0) return 0;
    if (strchr("=<>", token[0]) == NULL) {
        serror(0);
        return 0;
    }
    relation = token[0];
    get_token();
    if (token[0] == '\0') {
        serror(2);
    } else {
        level2(&right);
        putback_token();
    }
    if (relation == '<') condition = left < right;
    else if (relation == '=') condition = left == right;
    else if (relation == '>') condition = left > right;
    get_token();
    if (done != 0) return 0;
    if (tok != TOK_THEN) {
        serror(8);
        return 0;
    }
    if (condition) {
        get_token();
        if (done != 0) return 0;
        if (tok == TOK_ELSE) {
            find_eol();
            return 0;
        }
        while ((unsigned char)(tok - TOK_EOL) > 1) {
            exec_command();
            get_token();
            if (done != 0) return 0;
            if (tok == TOK_ELSE) {
                find_eol();
                return 0;
            }
        }
        if (tok == TOK_ELSE) find_eol();
        return 0;
    }
    do {
        if (tok == TOK_EOL) return 0;
        get_token();
        if (done != 0) return 0;
    } while (tok != TOK_ELSE);
    return 0;
}

/* 0xF2EF0, 773 bytes. */
int exec_rem(void)
{
    do {
        get_token();
        if (done != 0) return 0;
    } while (tok != TOK_EOL);
    return 0;
}

/* 0xF3200, 42 bytes. */
int find_eol(void)
{
    while (*prog != '\n' && *prog != '\0') ++prog;
    return 0;
}

/* 0xF3230, 102 bytes. */
char *find_label(char *name)
{
    int i;
    for (i = 0; i < SBASIC_LINES; ++i) {
        if (strcmp(label_table[i].name, name) == 0) {
            return label_table[i].p;
        }
    }
    return NULL;
}

/* 0xF32A0, 68 bytes. */
float find_var(char *name)
{
    if (!isalpha((unsigned char)*name)) {
        serror(4);
        return 0.0f;
    }
    return variables[variable_index(token[0])];
}

/* 0xF32F0, 87 bytes. */
int for_complete(for_stack *entry)
{
    float value = variables[(int)entry->var];
    if (entry->step > 0.0f) {
        return entry->target <= value && entry->target != value;
    }
    if (entry->step < 0.0f) {
        return value <= entry->target && entry->target != value;
    }
    return 0;
}

/* 0xF3350, 59 bytes. */
for_stack *fpop(void)
{
    --ftos;
    if (ftos < 0) serror(11);
    return &fstack[ftos];
}

/* 0xF3390, 87 bytes. */
void fpush(for_stack *entry)
{
    if (ftos > SBASIC_STACK_DEPTH) serror(10);
    fstack[ftos++] = *entry;
}

/* 0xF33F0, 105 bytes. */
void get_exp(float *result)
{
    get_token();
    if (token[0] == '\0') {
        serror(2);
        return;
    }
    level2(result);
    putback_token();
}

/* 0xF3460, 118 bytes. */
int get_next_label(char *name)
{
    int i;
    for (i = 0; i < SBASIC_LINES; ++i) {
        if (label_table[i].name[0] == '\0') return i;
        if (strcmp(label_table[i].name, name) == 0) return -2;
    }
    return -1;
}

/* 0xF34E0, 784 bytes. */
int get_token(void)
{
    char *destination;
    char current;
    int i;

    tok = 0;
    token_type = 0;
    destination = token;
    if ((unsigned char)(*prog + 1) < 2) {
        token[0] = '\0';
        tok = TOK_FINISHED;
        token_type = TOKEN_DELIMITER;
        return TOKEN_DELIMITER;
    }
    while (*prog == ' ' || *prog == '\t') ++prog;
    if (*prog == '\n') {
        tok = TOK_EOL;
        ++prog;
        token[0] = '\n';
        token[1] = '\0';
        token_type = TOKEN_DELIMITER;
        return TOKEN_DELIMITER;
    }
    if (strchr("+-*^/%=;(),<>", *prog) != NULL) {
        token[0] = *prog++;
        token[1] = '\0';
        token_type = TOKEN_DELIMITER;
        return TOKEN_DELIMITER;
    }
    if (*prog == '"') {
        current = *++prog;
        while (current != '"' && current != '\r') {
            *destination++ = current;
            current = *++prog;
        }
        if (current != '\n') {
            *destination = '\0';
            ++prog;
            token_type = TOKEN_QUOTED;
            return TOKEN_QUOTED;
        }
        serror(1);
        return -1;
    }
    if (isdigit((unsigned char)*prog)) {
        while (!isdelim(*prog)) *destination++ = *prog++;
        *destination = '\0';
        token_type = TOKEN_NUMBER;
        return TOKEN_NUMBER;
    }
    if (isalpha((unsigned char)*prog)) {
        while (!isdelim(*prog)) *destination++ = *prog++;
        token_type = TOKEN_IDENTIFIER;
    }
    *destination = '\0';
    if (token_type == TOKEN_IDENTIFIER) {
        for (destination = token; *destination != '\0'; ++destination) {
            *destination = (char)tolower((unsigned char)*destination);
        }
        i = look_up(token);
        tok = (char)i;
        if (tok != 0) {
            token_type = TOKEN_COMMAND;
            return TOKEN_COMMAND;
        }
        token_type = TOKEN_VARIABLE;
    }
    return token_type;
}

/* 0xF37F0, 253 bytes. */
int gosub(void)
{
    char *position;
    get_token();
    if (done != 0) return 0;
    position = find_label(token);
    if (position == NULL) {
        serror(7);
        return 0;
    }
    gpush(prog);
    prog = position;
    return 0;
}

/* 0xF38F0, 59 bytes. */
char *gpop(void)
{
    if (gtos == 0) {
        serror(13);
        return NULL;
    }
    return gstack[gtos--];
}

/* 0xF3930, 58 bytes. */
int gpush(char *position)
{
    ++gtos;
    if (gtos == SBASIC_STACK_DEPTH) {
        serror(gtos - 13);
        return 0;
    }
    gstack[gtos] = position;
    return 0;
}

/* 0xF3970, 74 bytes. */
int greturn(void)
{
    if (gtos == 0) {
        serror(13);
        prog = NULL;
        return 0;
    }
    prog = gstack[gtos--];
    return 0;
}

/* 0xF39C0, 158 bytes. */
int input(void)
{
    get_token();
    if (done != 0) return 0;
    if (token_type == TOKEN_QUOTED) {
        printf("%s", token);
        get_token();
        if (done != 0) return 0;
        if (token[0] == ',' || token[0] == ';') get_token();
        else serror(1);
        if (done != 0) return 0;
    } else {
        printf("? ");
    }
    variables[variable_index(token[0])] = 0.0f;
    return 0;
}

/* 0xF3A60, 59 bytes. */
int isdelim(char value)
{
    return strchr(" ;,+-<>/*%^=()", value) != NULL ||
        value == '\t' || value == '\n' || value == '\0';
}

/* 0xF3AA0, 20 bytes. */
int iswhite(char value)
{
    return value == ' ' || value == '\t';
}

/* 0xF3AC0, 31 bytes. */
int label_init(void)
{
    int i;
    for (i = 0; i < SBASIC_LINES; ++i) label_table[i].name[0] = '\0';
    return 0;
}

/* 0xF3AE0, 150 bytes. */
void level2(float *result)
{
    char op;
    float hold;
    level3(result);
    while (token[0] == '+' || token[0] == '-') {
        op = token[0];
        get_token();
        level3(&hold);
        arith(op, result, &hold);
    }
}

/* 0xF3B80, 101 bytes. */
void level3(float *result)
{
    char op;
    float hold;
    level4(result);
    while (strchr("*/%", token[0]) != NULL && token[0] != '\0') {
        op = token[0];
        get_token();
        level4(&hold);
        arith(op, result, &hold);
    }
}

/* 0xF3BF0, 306 bytes. */
void level4(float *result)
{
    char op = 0;
    float hold;
    if ((token_type == TOKEN_DELIMITER && token[0] == '+') ||
        token[0] == '-') {
        op = token[0];
        get_token();
    }
    if (token[0] == '(' && token_type == TOKEN_DELIMITER) {
        get_token();
        level2(result);
        if (token[0] != ')') {
            serror(1);
            goto apply_unary;
        }
    } else if (token_type == TOKEN_VARIABLE) {
        if (!isalpha((unsigned char)token[0])) {
            serror(4);
            *result = 0.0f;
        } else {
            *result = variables[variable_index(token[0])];
        }
    } else if (token_type == TOKEN_NUMBER) {
        *result = (float)atof(token);
    } else {
        serror(0);
        goto apply_unary;
    }
    get_token();
apply_unary:
    if (op == '-') unary(op, result);
    while (token[0] == '^') {
        get_token();
        level4(&hold);
        arith('^', result, &hold);
    }
}

/* 0xF3D30, 253 bytes. */
void level5(float *result)
{
    char op = 0;
    if ((token_type == TOKEN_DELIMITER && token[0] == '+') ||
        token[0] == '-') {
        op = token[0];
        get_token();
    }
    if (token[0] == '(' && token_type == TOKEN_DELIMITER) {
        get_token();
        level2(result);
        if (token[0] != ')') {
            serror(1);
            goto apply_unary;
        }
    } else if (token_type == TOKEN_VARIABLE) {
        if (!isalpha((unsigned char)token[0])) {
            serror(4);
            *result = 0.0f;
        } else {
            *result = variables[variable_index(token[0])];
        }
    } else if (token_type == TOKEN_NUMBER) {
        *result = (float)atof(token);
    } else {
        serror(0);
        goto apply_unary;
    }
    get_token();
apply_unary:
    if (op == '-') unary(op, result);
}

/* 0xF3E30, 211 bytes. */
void level6(float *result)
{
    if (token[0] == '(' && token_type == TOKEN_DELIMITER) {
        get_token();
        level2(result);
        if (token[0] != ')') {
            serror(1);
            return;
        }
    } else if (token_type == TOKEN_VARIABLE) {
        if (!isalpha((unsigned char)token[0])) {
            serror(4);
            *result = 0.0f;
        } else {
            *result = variables[variable_index(token[0])];
        }
    } else if (token_type == TOKEN_NUMBER) {
        *result = (float)atof(token);
    } else {
        serror(0);
        return;
    }
    get_token();
}

/* 0xF3F10, 149 bytes. */
int look_up(char *name)
{
    int i;
    char *scan;
    for (scan = name; *scan != '\0'; ++scan) {
        *scan = (char)tolower((unsigned char)*scan);
    }
    for (i = 0; table[i].command[0] != '\0'; ++i) {
        if (strcmp(table[i].command, name) == 0) return table[i].tok;
    }
    return 0;
}

/* 0xF3FB0, 247 bytes. */
int next(void)
{
    for_stack *entry = fpop();
    int variable = (int)entry->var;
    variables[variable] += entry->step;
    if (!for_complete(entry)) {
        fpush(entry);
        prog = entry->loc;
    }
    return 0;
}

/* 0xF40B0, 157 bytes. */
void primitive(float *result)
{
    if (token_type == TOKEN_VARIABLE) {
        if (!isalpha((unsigned char)token[0])) {
            serror(4);
            *result = 0.0f;
        } else {
            *result = variables[variable_index(token[0])];
        }
    } else if (token_type == TOKEN_NUMBER) {
        *result = (float)atof(token);
    } else {
        serror(0);
        return;
    }
    get_token();
}

/* 0xF4150, 445 bytes. */
int print(void)
{
    char last_delimiter = 0;
    float answer;
    unsigned length = 0;

    do {
        int printed;
        get_token();
        if (done != 0) return 0;
        if (token_type == TOKEN_QUOTED) {
            console_Printf(token);
            printed = (int)strlen(token);
            needoutput = 1;
            get_token();
            if (done != 0) return 0;
        } else {
            putback_token();
            get_token();
            if (token[0] == '\0') serror(2);
            else {
                level2(&answer);
                putback_token();
            }
            get_token();
            if (done != 0) return 0;
            printed = console_Printf("%f", answer);
            needoutput = 1;
        }
        last_delimiter = token[0];
        length += (unsigned)printed;
        if (token[0] == ',') {
            int spaces = 8 - (int)(length % 8);
            length += (unsigned)spaces;
            needoutput = 1;
            while (spaces-- != 0) console_Printf(" ");
        } else if (token[0] != ';') {
            break;
        }
    } while (token[0] == ';' || token[0] == ',');
    if (last_delimiter != ';' && last_delimiter != ',') {
        needoutput = 1;
        console_Printf("\n");
    }
    return 0;
}

/* 0xF4310, 52 bytes. */
void putback(void)
{
    putback_token();
}

/* 0xF4350, 428 bytes. */
int robanim(void)
{
    char *commandargs[16] = {0};
    float commandfloat[16] = {0};
    int commandint[16] = {0};
    float value;
    int count = 1;

    commandargs[0] = "play";
    do {
        get_token();
        if (done != 0) return 0;
        if (token_type == TOKEN_COMMAND && tok == TOK_PLAY) {
            do {
                parse_expression_argument(&value);
                commandint[count++] = (int)value;
            } while (done == 0 && count < 8 &&
                !(token_type == TOKEN_DELIMITER && tok == TOK_EOL));
            console_AnimCommand(
                count + 1, commandargs, commandint, commandfloat);
        }
    } while (token[0] == ';' || token[0] == ',');
    return 1;
}

/* 0xF4500, 387 bytes. */
int robgraphic(int type)
{
    char *commandargs[16];
    float commandfloat[16];
    int commandint[16] = {0};
    float value;
    int count = 0;

    do {
        parse_expression_argument(&value);
        commandint[count++] = (int)value;
    } while (done == 0 &&
        !(token_type == TOKEN_DELIMITER && tok == TOK_EOL) && count < 16);
    if (type == TOK_LINE) {
        console_LineCommand(count, commandargs, commandint, commandfloat);
    } else if (type == TOK_POINT) {
        console_PointCommand(count, commandargs, commandint, commandfloat);
    } else if (type == TOK_CIRCLE) {
        console_CircleCommand(count, commandargs, commandint, commandfloat);
    } else if (type == TOK_MOVE) {
        console_MoveCommand(count, commandargs, commandint, commandfloat);
    }
    return 1;
}

/* 0xF4690, 273 bytes. */
int robnode(void)
{
    char *commandargs[16];
    float commandfloat[16] = {0};
    int commandint[16];
    float value;
    int count = 0;

    do {
        parse_expression_argument(&value);
        commandfloat[count++] = value;
    } while (done == 0 &&
        !(token_type == TOKEN_DELIMITER && tok == TOK_EOL));
    console_NodeCommand(count, commandargs, commandint, commandfloat);
    return 1;
}

/* 0xF47B0, 1078 bytes. */
int robprint(void)
{
    char buffer[256];
    char output[256];
    float x;
    float y;
    float scale;
    float answer;
    unsigned length = 0;
    int iteration = 0;

    memset(buffer, 0, sizeof(buffer));
    memset(output, 0, sizeof(output));
    parse_expression_argument(&x);
    parse_expression_argument(&y);
    parse_expression_argument(&scale);
    do {
        int item_length;
        if (iteration != 0) get_token();
        if (done != 0) return 0;
        ++iteration;
        if (token_type == TOKEN_QUOTED) {
            strcat(output, token);
            needoutput = 1;
            item_length = (int)strlen(token);
            get_token();
            if (done != 0) return 0;
        } else {
            putback_token();
            get_token();
            if (token[0] == '\0') serror(2);
            else {
                level2(&answer);
                putback_token();
            }
            get_token();
            if (done != 0) return 0;
            sprintf(buffer, "%f", answer);
            strcat(output, buffer);
            needoutput = 1;
            item_length = (int)strlen(buffer);
        }
        length += (unsigned)item_length;
        if (token[0] == ',') {
            int spaces = 8 - (int)(length % 8);
            length += (unsigned)spaces;
            needoutput = 1;
            while (spaces-- != 0) strcat(output, " ");
        } else if (token[0] != ';') {
            break;
        }
    } while (token[0] == ';' || token[0] == ',');
    if (token[0] != ';' && token[0] != ',') {
        needoutput = 1;
        strcat(output, "\n");
    }
    _DrawText(x, y, 0.0001f, scale, UINT32_C(0x7FFFFFFF), "%s", output);
    return 0;
}

/* 0xF4BF0, 499 bytes. */
int scan_labels(void)
{
    char *saved = prog;
    int index;

    label_init();
    get_token();
    if (done == 0) {
        if (token_type == TOKEN_NUMBER) {
            strncpy(label_table[0].name, token, SBASIC_LABEL_NAME_BYTES);
            label_table[0].p = prog;
        }
        find_eol();
        do {
            get_token();
            if (done != 0) return 0;
            if (token_type == TOKEN_NUMBER) {
                index = get_next_label(token);
                if (index == -1) serror(5);
                else if (index == -2) serror(6);
                strncpy(
                    label_table[index].name,
                    token,
                    SBASIC_LABEL_NAME_BYTES);
                label_table[index].p = prog;
            }
            if (tok != TOK_EOL) find_eol();
        } while (tok != TOK_FINISHED);
    }
    prog = saved;
    return 0;
}

/* 0xF4DF0, 160 bytes. */
void serror(int error)
{
    char temporary[256];
    char *end;
    strncpy(temporary, line, sizeof(temporary));
    end = temporary;
    while (*end != '\0' && *end != '\n') ++end;
    *end = '\0';
    console_Printf("%s :%s\n", e[error], temporary);
    done = 1;
}

/* 0xF4E90, 21 bytes. */
void unary(char op, float *result)
{
    if (op == '-') {
        uint32_t bits;
        memcpy(&bits, result, sizeof(bits));
        bits ^= UINT32_C(0x80000000);
        memcpy(result, &bits, sizeof(bits));
    }
}

#ifdef JPB_SBASIC_TESTING
void jpb_SbasicSetProgram(char *text)
{
    prog = text;
    line = text;
    done = 0;
    needoutput = 0;
}

char *jpb_SbasicProgramCursor(void) { return prog; }
float *jpb_SbasicVariables(void) { return variables; }
int jpb_SbasicDone(void) { return done; }
int jpb_SbasicNeedsOutput(void) { return needoutput; }
int jpb_SbasicForDepth(void) { return ftos; }
int jpb_SbasicGosubDepth(void) { return gtos; }
#endif
