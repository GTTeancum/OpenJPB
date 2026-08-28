#include "jpb/sbasic.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
            __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static char output[8192];
static char command_line[256];
static int held_key;
static int pressed_key;
static int pressed_button;
static char saved_name[64];
static unsigned char saved_data[512];
static unsigned long saved_size;
static char *load_text;
static int callback_kind;
static int callback_count;
static int callback_ints[16];
static float callback_floats[16];

enum {
    CALLBACK_NONE,
    CALLBACK_ANIM,
    CALLBACK_CIRCLE,
    CALLBACK_LINE,
    CALLBACK_MOVE,
    CALLBACK_POINT,
    CALLBACK_NODE
};

static int record_callback(int kind, int count, int *ints, float *floats)
{
    int index;
    callback_kind = kind;
    callback_count = count;
    for (index = 0; index < 16; ++index) {
        callback_ints[index] =
            ints != NULL && kind != CALLBACK_NODE ? ints[index] : 0;
        callback_floats[index] =
            floats != NULL && (kind == CALLBACK_ANIM || kind == CALLBACK_NODE)
                ? floats[index] : 0.0f;
    }
    return 0;
}

static void append_output(const char *text)
{
    strncat(output, text, sizeof(output) - strlen(output) - 1);
}

void console_AddString(char *text)
{
    append_output(text);
}

int console_Printf(char *format, ...)
{
    char buffer[1024];
    va_list arguments;
    int result;
    va_start(arguments, format);
    result = vsprintf(buffer, format, arguments);
    va_end(arguments);
    append_output(buffer);
    return result;
}

int console_ProcessCommandLine(char *command)
{
    strcpy(command_line, command);
    return 0;
}

void memcard_on(void) {}
void memcard_off(void) {}

int memcard_LoadFile(
    int card, char *filename, unsigned char **data, unsigned long *size)
{
    (void)card;
    strcpy(saved_name, filename);
    if (load_text == NULL) return -4;
    *size = (unsigned long)strlen(load_text) + 1;
    *data = (unsigned char *)malloc(*size);
    memcpy(*data, load_text, *size);
    return 0;
}

void memcard_SaveFile(
    int card, char *filename, unsigned char *data, unsigned long size)
{
    (void)card;
    strcpy(saved_name, filename);
    saved_size = size;
    memcpy(saved_data, data, size < sizeof(saved_data) ? size : sizeof(saved_data));
}

int KeyHeld(int key) { return key == held_key; }
int KeyPressed(int key) { return key == pressed_key; }
int padbuttonpressed(int pad, int button)
{
    return pad == 0 && button == pressed_button;
}

int console_AnimCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_ANIM, n, i, f);
}
int console_CircleCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_CIRCLE, n, i, f);
}
int console_LineCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_LINE, n, i, f);
}
int console_MoveCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_MOVE, n, i, f);
}
int console_PointCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_POINT, n, i, f);
}
int console_NodeCommand(int n, char **s, int *i, float *f)
{
    (void)s; return record_callback(CALLBACK_NODE, n, i, f);
}
int _DrawText(float x, float y, float z, float scale,
    unsigned long color, char *format, ...)
{
    (void)x; (void)y; (void)z; (void)scale; (void)color; (void)format;
    return 0;
}

static int test_lookup_and_tokenizer(void)
{
    char name[] = "print";
    char program[] = "  print 12.5,A;\"hi\"\n";

    CHECK(sizeof(for_stack) == 24);
    CHECK(sizeof(label) == 24);
    CHECK(look_up(name) == 1);
    CHECK(strcmp(name, "print") == 0);

    jpb_SbasicSetProgram(program);
    CHECK(get_token() == 4 && tok == 1 && strcmp(token, "print") == 0);
    CHECK(get_token() == 3 && strcmp(token, "12.5") == 0);
    CHECK(get_token() == 1 && token[0] == ',');
    CHECK(get_token() == 2 && strcmp(token, "a") == 0);
    CHECK(get_token() == 1 && token[0] == ';');
    CHECK(get_token() == 6 && strcmp(token, "hi") == 0);
    CHECK(get_token() == 1 && tok == 9 && token[0] == '\n');
    CHECK(get_token() == 1 && tok == 10 && token[0] == '\0');
    return 0;
}

static int test_expression_core(void)
{
    char expression[] = "2+3*4^2\n";
    float result = 0.0f;
    float left = 12.0f;
    float right = 5.0f;

    jpb_SbasicSetProgram(expression);
    get_exp(&result);
    CHECK(result == 50.0f);
    arith('%', &left, &right);
    CHECK(left == 0.0f);
    unary('-', &right);
    CHECK(right == -5.0f);
    return 0;
}

static int test_line_store_and_cards(void)
{
    output[0] = '\0';
    saved_name[0] = '\0';
    saved_size = 0;
    basic_Init();
    basic_AddLine("30 END");
    basic_AddLine("10 A=1");
    basic_AddLine("20 PRINT A");
    basic_List(0, 100);
    CHECK(strcmp(output, "10 A=1\n20 PRINT A\n30 END\n") == 0);

    output[0] = '\0';
    basic_AddLine("20");
    basic_List(0, 100);
    CHECK(strcmp(output, "10 A=1\n30 END\n") == 0);

    basic_SaveProgram("mixed.case", 3);
    CHECK(strcmp(saved_name, "MIXED.JPB") == 0);
    CHECK(saved_size == strlen("10 A=1\n30 END\n") + 1);
    CHECK(strcmp((char *)saved_data, "10 A=1\n") == 0);

    load_text = "5 A=9\n15 END\n";
    basic_LoadProgram("loaded.anything", 2);
    CHECK(strcmp(saved_name, "LOADED.JPB") == 0);
    output[0] = '\0';
    basic_List(0, 100);
    CHECK(strcmp(output, "5 A=9\n15 END\n") == 0);
    load_text = NULL;
    basic_New();
    return 0;
}

static int test_program_execution(void)
{
    int running;
    float *values;

    output[0] = '\0';
    basic_Init();
    basic_AddLine("10 A=2");
    basic_AddLine("20 FOR B=1 TO 3");
    basic_AddLine("30 A=A+B");
    basic_AddLine("40 NEXT");
    basic_AddLine("50 PRINT A");
    basic_AddLine("60 END");
    basic_PrepareToRun();
    running = basic_Run(0);
    CHECK(running == 1);
    values = jpb_SbasicVariables();
    CHECK(values[0] == 8.0f);
    CHECK(strstr(output, "8.000000") != NULL);
    CHECK(basic_Run(0) == 0);
    basic_New();
    return 0;
}

static int test_labels_yield_and_input(void)
{
    char expression[64];
    float *values = jpb_SbasicVariables();

    basic_Init();
    basic_AddLine("10 GOTO 30");
    basic_AddLine("20 A=99");
    basic_AddLine("30 A=7");
    basic_AddLine("40 CYCLE");
    basic_AddLine("50 END");
    basic_PrepareToRun();
    CHECK(basic_Run(0) == 1);
    CHECK(values[0] == 7.0f);
    CHECK(basic_Run(0) == 0);

    held_key = 'Q';
    strcpy(expression, "\"Q\"\n");
    jpb_SbasicSetProgram(expression);
    CHECK(bas_keyheld() == 1 && values[25] == 1.0f);
    pressed_key = 42;
    strcpy(expression, "42\n");
    jpb_SbasicSetProgram(expression);
    CHECK(bas_keypress() == 1 && values[25] == 1.0f);
    pressed_button = 6;
    strcpy(expression, "6\n");
    jpb_SbasicSetProgram(expression);
    CHECK(bas_joypad() == 1 && values[25] == 1.0f);
    basic_New();
    return 0;
}

static int test_system_command(void)
{
    char program[] = "\"gamma\",2\n";
    command_line[0] = '\0';
    jpb_SbasicSetProgram(program);
    consolesystem();
    CHECK(strcmp(command_line, "gamma 2.000000") == 0);
    return 0;
}

static int test_game_command_dispatch(void)
{
    char animation[] = "play 4 7\n";
    char line[] = "10 20 30 40\n";
    char point[] = "5 6\n";
    char circle[] = "1 2 3\n";
    char move[] = "8 9\n";
    char node[] = "1.5 2.25 3.75\n";

    callback_kind = CALLBACK_NONE;
    jpb_SbasicSetProgram(animation);
    CHECK(robanim() == 1);
    CHECK(callback_kind == CALLBACK_ANIM);
    CHECK(callback_count == 4);
    CHECK(callback_ints[1] == 4 && callback_ints[2] == 7);

    jpb_SbasicSetProgram(line);
    CHECK(robgraphic(22) == 1);
    CHECK(callback_kind == CALLBACK_LINE && callback_count == 4);
    CHECK(callback_ints[0] == 10 && callback_ints[3] == 40);

    jpb_SbasicSetProgram(point);
    CHECK(robgraphic(23) == 1);
    CHECK(callback_kind == CALLBACK_POINT && callback_count == 2);

    jpb_SbasicSetProgram(circle);
    CHECK(robgraphic(24) == 1);
    CHECK(callback_kind == CALLBACK_CIRCLE && callback_count == 3);

    jpb_SbasicSetProgram(move);
    CHECK(robgraphic(25) == 1);
    CHECK(callback_kind == CALLBACK_MOVE && callback_count == 2);

    jpb_SbasicSetProgram(node);
    CHECK(robnode() == 1);
    CHECK(callback_kind == CALLBACK_NODE && callback_count == 3);
    CHECK(callback_floats[0] == 1.5f && callback_floats[2] == 3.75f);
    return 0;
}

int main(void)
{
    CHECK(test_lookup_and_tokenizer() == 0);
    CHECK(test_expression_core() == 0);
    CHECK(test_line_store_and_cards() == 0);
    CHECK(test_program_execution() == 0);
    CHECK(test_labels_yield_and_input() == 0);
    CHECK(test_system_command() == 0);
    CHECK(test_game_command_dispatch() == 0);
    puts("sbasic tests passed");
    return 0;
}
