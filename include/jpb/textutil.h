#ifndef JPB_TEXTUTIL_H
#define JPB_TEXTUTIL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _TTF_Font _TTF_Font;

typedef int (*JPBTextUtilSetFontSizeHook)(
    _TTF_Font *font, int point_size);
typedef int (*JPBTextUtilGlyphMetricsHook)(
    _TTF_Font *font,
    unsigned short glyph,
    int *minimum_x,
    int *maximum_x,
    int *minimum_y,
    int *maximum_y,
    int *advance);
typedef const char *(*JPBTextUtilGetErrorHook)(void);

void jpb_TextUtilSetFontMetricsHooks(
    JPBTextUtilSetFontSizeHook set_font_size_hook,
    JPBTextUtilGlyphMetricsHook glyph_metrics_hook,
    JPBTextUtilGetErrorHook get_error_hook);
void ClearGlyphCache(void);
void ConvertToUTF16(const char *string, unsigned short **utfString);
void SizeText(
    _TTF_Font *font,
    int point_size,
    const unsigned short *string,
    int *width,
    int *height);

#ifdef __cplusplus
}

#include <string>
std::string toLower(std::string value);
#endif

#endif
