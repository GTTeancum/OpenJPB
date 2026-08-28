#include "jpb/console.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(stderr, "check failed at %s:%d: %s\n",                \
                    __FILE__, __LINE__, #condition);                         \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct SavedConsoleHeader {
    int alpha;
    int lines;
    int bindings;
} SavedConsoleHeader;

typedef struct TextCall {
    float x;
    float y;
    unsigned long color;
    char text[256];
} TextCall;

static unsigned char g_last_key;
static int g_shift_down;
static int g_basic_init_calls;
static int g_basic_add_line_calls;
static char g_basic_line[256];
static int g_basic_list_calls;
static int g_basic_list_from;
static int g_basic_list_to;
static int g_basic_load_calls;
static int g_basic_new_calls;
static int g_basic_prepare_calls;
static int g_basic_prepare_result;
static int g_basic_run_calls;
static int g_basic_run_result;
static int g_basic_save_calls;
static int g_memcard_on_calls;
static int g_memcard_off_calls;
static int g_memcard_load_result;
static unsigned char *g_memcard_load_data;
static unsigned long g_memcard_load_size;
static unsigned char g_saved_data[65536];
static unsigned long g_saved_size;
static int g_saved_card;
static char g_saved_filename[64];
static TextCall g_text_calls[96];
static int g_text_call_count;
static int g_rectangle_calls;
static float g_rectangle_x;
static float g_rectangle_y;
static float g_rectangle_width;
static float g_rectangle_height;
static long g_rectangle_color;
static int g_capture_calls;
static int g_capture_argc;
static char g_capture_args[16][64];
static int g_capture_ints[16];
static float g_capture_floats[16];

int debug_slomo;

static int nearly_equal(float a, float b)
{
    return fabsf(a - b) < 0.0001f;
}

static int capture_command(
    int argc, char **strings, int *integers, float *floats)
{
    int i;

    ++g_capture_calls;
    g_capture_argc = argc;
    for (i = 0; i < argc && i < 16; ++i) {
        strncpy(g_capture_args[i], strings[i], sizeof(g_capture_args[i]) - 1);
        g_capture_args[i][sizeof(g_capture_args[i]) - 1] = '\0';
        g_capture_ints[i] = integers[i];
        g_capture_floats[i] = floats[i];
    }
    return 73;
}

unsigned char LastKey(void)
{
    unsigned char key = g_last_key;
    g_last_key = 0;
    return key;
}

int ShiftKeyDown(void)
{
    return g_shift_down;
}

void basic_AddLine(char *line)
{
    ++g_basic_add_line_calls;
    strncpy(g_basic_line, line, sizeof(g_basic_line) - 1);
    g_basic_line[sizeof(g_basic_line) - 1] = '\0';
}

void basic_Init(void)
{
    ++g_basic_init_calls;
}

void basic_List(int from, int to)
{
    ++g_basic_list_calls;
    g_basic_list_from = from;
    g_basic_list_to = to;
}

void basic_LoadProgram(char *filename, int card)
{
    (void)filename;
    (void)card;
    ++g_basic_load_calls;
}

void basic_New(void)
{
    ++g_basic_new_calls;
}

int basic_PrepareToRun(void)
{
    ++g_basic_prepare_calls;
    return g_basic_prepare_result;
}

int basic_Run(int cycles)
{
    (void)cycles;
    ++g_basic_run_calls;
    return g_basic_run_result;
}

void basic_SaveProgram(char *filename, int card)
{
    (void)filename;
    (void)card;
    ++g_basic_save_calls;
}

void memcard_on(void)
{
    ++g_memcard_on_calls;
}

void memcard_off(void)
{
    ++g_memcard_off_calls;
}

int memcard_FileExists(int card, char *filename)
{
    (void)card;
    (void)filename;
    return 0;
}

int memcard_DeleteFile(int card, char *filename)
{
    (void)card;
    (void)filename;
    return 0;
}

char *memcard_FindFirstFile(int card)
{
    (void)card;
    return NULL;
}

char *memcard_FindNextFile(int card)
{
    (void)card;
    return NULL;
}

int memcard_LoadFile(
    int card, char *filename, unsigned char **data, unsigned long *size)
{
    (void)card;
    (void)filename;
    if (g_memcard_load_result == 0) {
        *data = (unsigned char *)malloc(g_memcard_load_size);
        memcpy(*data, g_memcard_load_data, g_memcard_load_size);
        *size = g_memcard_load_size;
    }
    return g_memcard_load_result;
}

void memcard_SaveFile(
    int card, char *filename, unsigned char *data, unsigned long size)
{
    g_saved_card = card;
    strncpy(g_saved_filename, filename, sizeof(g_saved_filename) - 1);
    g_saved_filename[sizeof(g_saved_filename) - 1] = '\0';
    g_saved_size = size;
    memcpy(g_saved_data, data, size);
}

int console_rectangle(
    float x, float y, float width, float height, long color)
{
    ++g_rectangle_calls;
    g_rectangle_x = x;
    g_rectangle_y = y;
    g_rectangle_width = width;
    g_rectangle_height = height;
    g_rectangle_color = color;
    return 0;
}

void console_text(float x, float y, unsigned long color, char *buffer)
{
    if (g_text_call_count < (int)(sizeof(g_text_calls) / sizeof(g_text_calls[0]))) {
        TextCall *call = &g_text_calls[g_text_call_count];
        call->x = x;
        call->y = y;
        call->color = color;
        strncpy(call->text, buffer, sizeof(call->text) - 1);
        call->text[sizeof(call->text) - 1] = '\0';
    }
    ++g_text_call_count;
}

int console_PowerCommand(
    int argc, char **strings, int *integers, float *floats)
{
    (void)argc;
    (void)strings;
    (void)integers;
    (void)floats;
    return 1;
}

static void send_key(unsigned char key, int shifted)
{
    g_last_key = key;
    g_shift_down = shifted;
    console_HandleConsole();
}

static void clear_draw_calls(void)
{
    g_text_call_count = 0;
    g_rectangle_calls = 0;
}

static int check_layout_and_hex(void)
{
    CHECK(sizeof(ConsoleCommand) == 24);
    CHECK(sizeof(ConsoleKeyBinding) == 256);
    CHECK(ascii2hex("0") == 0);
    CHECK(ascii2hex("12aF") == 0x12afUL);
    CHECK(ascii2hex("1z") == 0x10UL);
    CHECK(ascii2hex("f-") == 0xf0UL);
    return 0;
}

static int check_registry_limit(void)
{
    static char names[256][16];
    static char shorts[256][16];
    int i;

    jpb_ConsoleResetCommands();
    for (i = 0; i < 255; ++i) {
        sprintf(names[i], "command%03d", i);
        sprintf(shorts[i], "c%03d", i);
        CHECK(console_AddCommand(names[i], shorts[i], capture_command) == 0);
    }
    CHECK(jpb_ConsoleCommandCount() == 255);
    CHECK(console_AddCommand("overflow", "ov", capture_command) == 0);
    CHECK(jpb_ConsoleCommandCount() == 255);
    CHECK(console_AddCommand(names[10], "different", capture_command) == 1);
    CHECK(console_AddCommand("different", shorts[10], capture_command) == 0);
    CHECK(jpb_ConsoleCommandCount() == 255);
    return 0;
}

static int check_initialization_and_gate(void)
{
    static const char *expected[] = {
        "help", "cls", "color", "dump", "history", "key", "card",
        "savekeys", "loadkeys", "dir", "list", "run", "new", "save",
        "load", "exit", "power"
    };
    char disabled[] = "help";
    char enable[] = "enable";
    char command[] = "capture \"two words\" -17 3.5";
    char short_command[] = "cap 9";
    size_t i;

    console_HandleConsole();
    CHECK(g_basic_init_calls == 1);
    CHECK(jpb_ConsoleCommandCount() == 17);
    CHECK(jpb_ConsoleVisibleLines() == 24);
    CHECK(jpb_ConsoleAlpha() == 176);
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        CHECK(strcmp(jpb_ConsoleCommandName(i), expected[i]) == 0);
    }

    CHECK(console_AddCommand(
              "enable", "en", console_EnableConsoleCommand) == 0);
    CHECK(console_AddCommand("capture", "cap", capture_command) == 0);
    console_enabled = 0;
    g_capture_calls = 0;
    (void)console_ProcessCommandLine(disabled);
    CHECK(g_capture_calls == 0);
    CHECK(console_enabled == 0);
    (void)console_ProcessCommandLine(enable);
    CHECK(console_enabled == 1);
    CHECK(console_ProcessCommandLine(command) == 73);
    CHECK(g_capture_calls == 1);
    CHECK(g_capture_argc == 3);
    CHECK(strcmp(g_capture_args[0], "two words") == 0);
    CHECK(g_capture_ints[1] == -17);
    CHECK(nearly_equal(g_capture_floats[2], 3.5f));
    CHECK(console_ProcessCommandLine(short_command) == 73);
    CHECK(g_capture_calls == 2 && g_capture_ints[0] == 9);
    return 0;
}

static int check_buffer_and_history(void)
{
    char long_line[236];
    int row;

    console_Cls();
    row = jpb_ConsoleBufferRow();
    memset(long_line, 'A', sizeof(long_line) - 1);
    long_line[sizeof(long_line) - 1] = '\0';
    (void)console_Printf("%s", long_line);
    CHECK(strlen(jpb_ConsoleBufferLine((size_t)row)) == 234);
    CHECK(strcmp(jpb_ConsoleBufferLine((size_t)((row + 1) & 255)), "A") == 0);
    CHECK(jpb_ConsoleBufferColumn() == 1);
    console_BackSpace();
    CHECK(jpb_ConsoleBufferColumn() == 0);
    console_BackSpace();
    CHECK(jpb_ConsoleBufferRow() == row);
    CHECK(jpb_ConsoleBufferColumn() == 234);

    console_Cls();
    console_ResetCommandLine();
    console_ShowConsole();
    send_key('c', 0);
    send_key('a', 0);
    send_key('p', 0);
    send_key(' ', 0);
    send_key('4', 0);
    send_key(13, 0);
    CHECK(g_capture_ints[0] == 4);
    send_key(0x1e, 0);
    CHECK(strcmp(
              jpb_ConsoleBufferLine((size_t)jpb_ConsoleBufferRow()),
              "]cap 4") == 0);
    send_key(0x1f, 0);
    CHECK(strcmp(
              jpb_ConsoleBufferLine((size_t)jpb_ConsoleBufferRow()),
              "]") == 0);
    return 0;
}

static int check_key_bindings_and_persistence(void)
{
    char *bind_f1[] = {"F1", "capture", "11"};
    char *bind_a[] = {"a", "capture", "12"};
    char *remove_f1[] = {"F1", "remove"};
    SavedConsoleHeader *header;
    unsigned char load_blob[sizeof(SavedConsoleHeader) + sizeof(ConsoleKeyBinding)];
    ConsoleKeyBinding *saved_binding;
    const ConsoleKeyBinding *binding;

    (void)console_CommandKey(3, bind_f1, NULL, NULL);
    (void)console_CommandKey(3, bind_a, NULL, NULL);
    binding = jpb_ConsoleKeyBinding(0);
    CHECK(binding->key == 0xf1);
    CHECK(strcmp(binding->commandline, "capture 11") == 0);
    CHECK(jpb_ConsoleKeyBinding(1)->key == 'a');

    g_saved_size = 0;
    (void)console_CommandSaveKeys(0, NULL, NULL, NULL);
    CHECK(g_saved_card == 0);
    CHECK(strcmp(g_saved_filename, "JPBKEYS.JPB") == 0);
    CHECK(g_saved_size == sizeof(SavedConsoleHeader) +
                          2 * sizeof(ConsoleKeyBinding));
    header = (SavedConsoleHeader *)(void *)g_saved_data;
    CHECK(header->alpha == jpb_ConsoleAlpha());
    CHECK(header->lines == jpb_ConsoleVisibleLines());
    CHECK(header->bindings == 2);

    (void)console_CommandKey(2, remove_f1, NULL, NULL);
    g_saved_size = 0;
    (void)console_CommandSaveKeys(0, NULL, NULL, NULL);
    header = (SavedConsoleHeader *)(void *)g_saved_data;
    CHECK(header->bindings == 0);
    CHECK(g_saved_size == sizeof(SavedConsoleHeader));

    header = (SavedConsoleHeader *)(void *)load_blob;
    header->alpha = 99;
    header->lines = 7;
    header->bindings = 1;
    saved_binding = (ConsoleKeyBinding *)(void *)(load_blob + sizeof(*header));
    memset(saved_binding, 0, sizeof(*saved_binding));
    strcpy(saved_binding->commandline, "capture 21");
    saved_binding->key = 0xf2;
    g_memcard_load_data = load_blob;
    g_memcard_load_size = sizeof(load_blob);
    g_memcard_load_result = 0;
    (void)console_CommandLoadKeys(0, NULL, NULL, NULL);
    CHECK(jpb_ConsoleAlpha() == 99);
    CHECK(jpb_ConsoleVisibleLines() == 7);
    CHECK(jpb_ConsoleKeyBinding(0)->key == 0xf2);
    CHECK(strcmp(jpb_ConsoleKeyBinding(0)->commandline, "capture 21") == 0);
    CHECK(jpb_ConsoleKeyBinding(1)->key == 0);
    CHECK(g_memcard_on_calls == g_memcard_off_calls);
    return 0;
}

static int check_display_and_controls(void)
{
    int i;

    console_Cls();
    (void)console_Printf("visible");
    console_ShowConsole();
    debug_slomo = 0;
    clear_draw_calls();
    console_Display();
    CHECK(g_text_call_count == 7 || g_text_call_count == 8);
    CHECK(nearly_equal(g_text_calls[0].x, 20.0f));
    CHECK(nearly_equal(g_text_calls[0].y, 96.0f));
    CHECK(g_text_calls[0].color == 0x63f0f0f0UL);
    CHECK(strcmp(g_text_calls[0].text, "visible") == 0);
    CHECK(g_rectangle_calls == 1);
    CHECK(nearly_equal(g_rectangle_x, 0.0f));
    CHECK(nearly_equal(g_rectangle_y, 0.0f));
    CHECK(nearly_equal(g_rectangle_width, 640.0f));
    CHECK(nearly_equal(g_rectangle_height, 112.0f));
    CHECK((unsigned long)g_rectangle_color == 0x63000000UL);

    debug_slomo = 1;
    clear_draw_calls();
    console_Display();
    CHECK(g_text_call_count == 0 && g_rectangle_calls == 0);
    debug_slomo = 0;

    for (i = 0; i < 20; ++i) {
        send_key(0x1c, 1);
    }
    CHECK(jpb_ConsoleAlpha() == 0);
    for (i = 0; i < 20; ++i) {
        send_key(0x1d, 1);
    }
    CHECK(jpb_ConsoleAlpha() == 255);
    for (i = 0; i < 80; ++i) {
        send_key(0x1e, 1);
    }
    CHECK(jpb_ConsoleVisibleLines() == 2);
    for (i = 0; i < 80; ++i) {
        send_key(0x1f, 1);
    }
    CHECK(jpb_ConsoleVisibleLines() == 65);
    send_key(0x95, 0);
    CHECK(jpb_ConsoleScrollDestination() == 191);
    send_key(0x96, 0);
    CHECK(jpb_ConsoleScrollDestination() == 0);
    return 0;
}

static int check_basic_and_keyboard_ownership(void)
{
    char numbered[] = "10 PRINT 1";
    char *list_args[] = {"4", "9"};
    int list_ints[] = {4, 9};

    console_basicrunning = 0;
    (void)console_ProcessCommandLine(numbered);
    CHECK(g_basic_add_line_calls == 1);
    CHECK(strcmp(g_basic_line, "10 PRINT 1") == 0);
    CHECK(console_CommandList(2, list_args, list_ints, NULL) == 1);
    CHECK(g_basic_list_calls == 1);
    CHECK(g_basic_list_from == 4 && g_basic_list_to == 9);

    g_basic_prepare_result = 17;
    CHECK(console_CommandRun(0, NULL, NULL, NULL) == 17);
    CHECK(g_basic_prepare_calls == 1);
    CHECK(console_basicrunning == 1);
    g_basic_run_result = 1;
    send_key(0, 0);
    CHECK(g_basic_run_calls == 1 && console_basicrunning == 1);
    send_key(27, 0);
    CHECK(console_basicrunning == 0);

    bKeyboardNabbed = 0;
    CHECK(console_NabKeyboard() == 1);
    CHECK(console_NabKeyboard() == 0);
    g_last_key = '`';
    console_HandleConsole();
    CHECK(g_last_key == 0);
    CHECK(console_ReleaseKeyboard() == 0);
    CHECK(bKeyboardNabbed == 0);
    return 0;
}

int main(void)
{
    CHECK(check_layout_and_hex() == 0);
    CHECK(check_registry_limit() == 0);
    CHECK(check_initialization_and_gate() == 0);
    CHECK(check_buffer_and_history() == 0);
    CHECK(check_key_bindings_and_persistence() == 0);
    CHECK(check_display_and_controls() == 0);
    CHECK(check_basic_and_keyboard_ownership() == 0);
    puts("Console reconstruction tests passed");
    return 0;
}
