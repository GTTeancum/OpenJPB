/*
 * COMPLETE REVIEWED RECONSTRUCTION from
 * W:\SWJediPowerBattles\Work\alltext.c.
 *
 * The font-selection leaves, localization aggregate, and generateAllText
 * publication path are recovered. The retail aggregate stores UTF-8 byte
 * strings behind a wchar_t* PDB type and publishes those pointers directly.
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

/* Exact initialized PDB globals at RVAs 0x4A6528..0x4A6550. */
const char *SC_ITALIC_FS_FILENAME = "NotoSansSC-Light.ttf";
const char *SC_REGULAR_FS_FILENAME = "NotoSansSC-Regular.ttf";
const char *SC_BOLD_FS_FILENAME = "NotoSansSC-Bold.ttf";
const char *DEFAULT_ITALIC_FS_FILENAME = "NotoSans-Italic.ttf";
const char *DEFAULT_REGULAR_FS_FILENAME = "NotoSansSC-Regular.ttf";
const char *DEFAULT_BOLD_FS_FILENAME = "NotoSansSC-Bold.ttf";

/* Exact PDB global at matched-PC RVA 0x10DEBA8. */
int currentLanguage;

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
        const char *text = allTextEverything[
            index < JPB_ALL_TEXT_SHARED_COUNT
                ? index
                : language * JPB_ALL_TEXT_LANGUAGE_TAIL_COUNT + index];

        allText[index] = (char *)text;
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
