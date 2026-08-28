#include "jpb/console.h"
#include "jpb/main.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

static size_t row_offset(size_t first, size_t offset)
{
    return (size_t)(uint8_t)(first + offset);
}

int main(void)
{
    char question[] = "?";
    char frames[] = "7";
    char *question_arguments[] = {question};
    char *frame_arguments[] = {frames};
    int integer_arguments[] = {7};
    size_t first_row;

    card_gInitVariables();

    debug_slomo = 0;
    assert(console_CommandPause(0, NULL, NULL, NULL) == 1);
    assert(debug_slomo == 1);
    assert(console_CommandPause(0, NULL, NULL, NULL) == 0);
    assert(debug_slomo == 0);

    jpb_ConsoleResetCommands();
    first_row = (size_t)(uint8_t)jpb_ConsoleBufferRow();
    (void)console_CommandPause(1, question_arguments, NULL, NULL);
    assert(strcmp(
               jpb_ConsoleBufferLine(first_row),
               "PAUSE - [un]freeze the game (see step)") == 0);
    assert(strcmp(
               jpb_ConsoleBufferLine(row_offset(first_row, 1)),
               "usage: pause") == 0);
    assert(strcmp(
               jpb_ConsoleBufferLine(row_offset(first_row, 2)),
               "pause and step are only useful when bound to keys...") == 0);

    debug_singlestep = 0;
    (void)console_CommandStep(0, NULL, NULL, NULL);
    assert(debug_singlestep == 1);
    assert(console_CommandStep(
               1, frame_arguments, integer_arguments, NULL) == 7);
    assert(debug_singlestep == 7);

    jpb_ConsoleResetCommands();
    first_row = (size_t)(uint8_t)jpb_ConsoleBufferRow();
    (void)console_CommandStep(
        1, question_arguments, integer_arguments, NULL);
    assert(strcmp(
               jpb_ConsoleBufferLine(first_row),
               "STEP - singlestep the game when frozen (see pause)") == 0);
    assert(strcmp(
               jpb_ConsoleBufferLine(row_offset(first_row, 1)),
               "has no effect if game not frozen") == 0);
    assert(strcmp(
               jpb_ConsoleBufferLine(row_offset(first_row, 2)),
               "usage: step [frames]") == 0);
    assert(strcmp(
               jpb_ConsoleBufferLine(row_offset(first_row, 3)),
               "pause and step are only useful when bound to keys...") == 0);

    return 0;
}
