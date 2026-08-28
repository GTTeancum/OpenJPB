#include "jpb/textutil.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

static int set_size_calls;
static int glyph_metric_calls;
static int get_error_calls;
static int set_size_result;

static int set_test_font_size(_TTF_Font *font, int point_size)
{
    assert(font != nullptr);
    assert(point_size == 24);
    ++set_size_calls;
    return set_size_result;
}

static const char *get_test_error()
{
    ++get_error_calls;
    return "test SDL error";
}

static int get_test_glyph_metrics(
    _TTF_Font *font,
    unsigned short glyph,
    int *minimum_x,
    int *maximum_x,
    int *minimum_y,
    int *maximum_y,
    int *advance)
{
    assert(font != nullptr);
    ++glyph_metric_calls;
    if (glyph == static_cast<unsigned short>('A')) {
        *minimum_x = 1;
        *maximum_x = 6;
        *minimum_y = -2;
        *maximum_y = 8;
        *advance = 7;
        return 0;
    }
    if (glyph == static_cast<unsigned short>('B')) {
        *minimum_x = -1;
        *maximum_x = 5;
        *minimum_y = -1;
        *maximum_y = 9;
        *advance = 6;
        return 0;
    }
    return -1;
}

int main()
{
    int font_token;
    _TTF_Font *font = reinterpret_cast<_TTF_Font *>(&font_token);
    static const unsigned short text_ab[] = {'A', 'B', 0};
    static const unsigned short text_multiline[] = {
        'A', 'B', '\n', 'A', 0};
    int width;
    int height;

    assert(toLower("WinIF2.TGA") == "winif2.tga");

    unsigned short *converted = nullptr;
    ConvertToUTF16("A\xC3\xA9", &converted);
    assert(converted != nullptr);
    assert(converted[0] == static_cast<unsigned short>('A'));
    assert(converted[1] == 0x00E9U);
    assert(converted[2] == 0U);
    std::free(converted);

    converted = static_cast<unsigned short *>(std::malloc(8));
    assert(converted != nullptr);
    ConvertToUTF16("\xFF", &converted);
    assert(converted != nullptr);
    assert(std::memcmp(converted, "ERROR", 6) == 0);
    std::free(converted);

    ClearGlyphCache();
    jpb_TextUtilSetFontMetricsHooks(
        set_test_font_size, get_test_glyph_metrics, get_test_error);
    SizeText(font, 24, text_ab, &width, &height);
    assert(width == 12);
    assert(height == 11);
    assert(set_size_calls == 1);
    assert(glyph_metric_calls == 2);

    SizeText(font, 24, text_multiline, &width, &height);
    assert(width == 5);
    assert(height == 21);
    assert(set_size_calls == 2);
    assert(glyph_metric_calls == 2);

    set_size_result = -1;
    static const unsigned short text_unknown[] = {'C', 0};
    SizeText(font, 24, text_unknown, &width, &height);
    assert(width == 0);
    assert(height == 0);
    assert(set_size_calls == 3);
    assert(glyph_metric_calls == 3);
    assert(get_error_calls == 2);
    set_size_result = 0;

    width = 71;
    height = 83;
    SizeText(nullptr, 24, text_ab, &width, &height);
    assert(width == 71);
    assert(height == 83);
    jpb_TextUtilSetFontMetricsHooks(nullptr, nullptr, nullptr);
    ClearGlyphCache();
    return 0;
}
