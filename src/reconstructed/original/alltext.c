/*
 * REVIEWED LEAVES from W:\SWJediPowerBattles\Work\alltext.c.
 *
 * The font-selection leaves, localization aggregate, and generateAllText
 * publication path are recovered. The retail aggregate stores UTF-8 byte
 * strings behind a wchar_t* PDB type; this portable reconstruction widens
 * those bytes once per language so the reviewed text boundary has a stable,
 * native wchar_t representation on every host.
 * PDB module: 0003
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\alltext.obj
 * Primary source: W:\SWJediPowerBattles\Work\alltext.c
 * Compiler language: c
 * Emitted procedures: 3
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/alltext.h"

#include "jpb/menu.h"
#include "jpb/resources.h"
#include "jpb/text.h"
#include "jpb/whook.h"

#include <stdint.h>
#include <stdlib.h>

/* Exact initialized PDB globals at RVAs 0x4A6528..0x4A6550. */
const char *SC_ITALIC_FS_FILENAME = "NotoSansSC-Light.ttf";
const char *SC_REGULAR_FS_FILENAME = "NotoSansSC-Regular.ttf";
const char *SC_BOLD_FS_FILENAME = "NotoSansSC-Bold.ttf";
const char *DEFAULT_ITALIC_FS_FILENAME = "NotoSans-Italic.ttf";
const char *DEFAULT_REGULAR_FS_FILENAME = "NotoSansSC-Regular.ttf";
const char *DEFAULT_BOLD_FS_FILENAME = "NotoSansSC-Bold.ttf";

/* Exact PDB global at matched-PC RVA 0x10DEBA8. */
int currentLanguage;

static wchar_t *jpb_all_text_wide
    [JPB_ALL_TEXT_LANGUAGE_COUNT][JPB_ALL_TEXT_ENTRY_COUNT];

static uint32_t jpb_decode_utf8(const unsigned char **cursor)
{
    const unsigned char *text = *cursor;
    uint32_t codepoint;
    int continuation_count;
    int index;

    if (text[0] < 0x80) {
        *cursor = text + 1;
        return text[0];
    }
    if ((text[0] & 0xE0) == 0xC0) {
        codepoint = text[0] & 0x1F;
        continuation_count = 1;
    } else if ((text[0] & 0xF0) == 0xE0) {
        codepoint = text[0] & 0x0F;
        continuation_count = 2;
    } else if ((text[0] & 0xF8) == 0xF0) {
        codepoint = text[0] & 0x07;
        continuation_count = 3;
    } else {
        *cursor = text + 1;
        return 0xFFFD;
    }
    for (index = 1; index <= continuation_count; ++index) {
        if ((text[index] & 0xC0) != 0x80) {
            *cursor = text + 1;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (text[index] & 0x3F);
    }
    *cursor = text + continuation_count + 1;
    if (codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return 0xFFFD;
    }
    return codepoint;
}

static size_t jpb_wchar_units(uint32_t codepoint)
{
    return sizeof(wchar_t) == 2 && codepoint > 0xFFFF ? 2u : 1u;
}

static wchar_t *jpb_widen_utf8(const char *utf8)
{
    const unsigned char *cursor;
    size_t length = 0;
    size_t output_index = 0;
    wchar_t *output;

    if (utf8 == NULL) {
        return NULL;
    }
    cursor = (const unsigned char *)utf8;
    while (*cursor != 0) {
        length += jpb_wchar_units(jpb_decode_utf8(&cursor));
    }
    output = (wchar_t *)malloc((length + 1) * sizeof(*output));
    if (output == NULL) {
        return NULL;
    }
    cursor = (const unsigned char *)utf8;
    while (*cursor != 0) {
        uint32_t codepoint = jpb_decode_utf8(&cursor);

        if (sizeof(wchar_t) == 2 && codepoint > 0xFFFF) {
            codepoint -= 0x10000;
            output[output_index++] =
                (wchar_t)(0xD800 + (codepoint >> 10));
            output[output_index++] =
                (wchar_t)(0xDC00 + (codepoint & 0x3FF));
        } else {
            output[output_index++] = (wchar_t)codepoint;
        }
    }
    output[output_index] = L'\0';
    return output;
}

static wchar_t *jpb_localized_text(int language, int index)
{
    wchar_t **slot = &jpb_all_text_wide[language][index];

    if (*slot == NULL) {
        *slot = jpb_widen_utf8(jpb_AllTextUtf8(language, index));
    }
    return *slot;
}

/* 0x17640, 119 bytes, global, 2 named locals
 * generateAllText
 * PDB type: void (int)
 * Source: W:\SWJediPowerBattles\Work\alltext.c
 */
void generateAllText(int language)
{
    int index;

    MarkFontAtlasForRefresh();
    UpdateCurrentlyLoadedFont(language);
    currentLanguage = language;
    for (index = 0; index < JPB_ALL_TEXT_ENTRY_COUNT; ++index) {
        allText[index] = jpb_localized_text(language, index);
    }
}

/* 0x176C0, 29 bytes, global, 1 named locals
 * getDefaultFontFile
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\alltext.c
 */
char *getDefaultFontFile(int fontStyle)
{
    const char *font = DEFAULT_REGULAR_FS_FILENAME;

    if (fontStyle == 1) {
        font = DEFAULT_ITALIC_FS_FILENAME;
    }
    return (char *)resource_getPath(font, JPB_RESOURCE_FONT);
}

/* 0x176E0, 109 bytes, global, 1 named locals
 * getFontFile
 * PDB type: char* (int)
 * Source: W:\SWJediPowerBattles\Work\alltext.c
 */
char *getFontFile(int fontStyle)
{
    const char *font;

    if (currentLanguage == 6) {
        if (fontStyle == 1) {
            font = SC_ITALIC_FS_FILENAME;
        } else if (fontStyle == 2) {
            font = SC_BOLD_FS_FILENAME;
        } else {
            font = SC_REGULAR_FS_FILENAME;
        }
    } else if (fontStyle == 1) {
        font = DEFAULT_ITALIC_FS_FILENAME;
    } else if (fontStyle == 2) {
        font = DEFAULT_BOLD_FS_FILENAME;
    } else {
        font = DEFAULT_REGULAR_FS_FILENAME;
    }
    return (char *)resource_getPath(font, JPB_RESOURCE_FONT);
}
