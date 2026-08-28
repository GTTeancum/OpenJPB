#ifndef JPB_ALLTEXT_H
#define JPB_ALLTEXT_H

#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int currentLanguage;

extern const char *SC_ITALIC_FS_FILENAME;
extern const char *SC_REGULAR_FS_FILENAME;
extern const char *SC_BOLD_FS_FILENAME;
extern const char *DEFAULT_ITALIC_FS_FILENAME;
extern const char *DEFAULT_REGULAR_FS_FILENAME;
extern const char *DEFAULT_BOLD_FS_FILENAME;

enum {
    JPB_ALL_TEXT_LANGUAGE_COUNT = 7,
    JPB_ALL_TEXT_ENTRY_COUNT = 498,
    JPB_ALL_TEXT_SHARED_COUNT = 127,
    JPB_ALL_TEXT_LANGUAGE_TAIL_COUNT = 371,
    JPB_ALL_TEXT_STORAGE_COUNT = 2725
};

/* Exact PDB aggregate; its pointers target UTF-8 bytes despite the PDB type. */
extern const char *allTextEverything[JPB_ALL_TEXT_STORAGE_COUNT];

void generateAllText(int language);
char *getDefaultFontFile(int fontStyle);
char *getFontFile(int fontStyle);

/* Recovered UTF-8 localization aggregate used by portable text startup. */
const char *jpb_AllTextUtf8(int language, int index);

#ifdef __cplusplus
}
#endif

#endif
