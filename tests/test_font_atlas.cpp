#include "jpb/font_atlas.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "check failed at %s:%d: %s\n",            \
                         __FILE__, __LINE__, #condition);                    \
            return 1;                                                        \
        }                                                                    \
    } while (0)

struct AtlasTrace {
    int createSurfaceCalls;
    int freeAtlasCalls;
    int freeGlyphCalls;
    int lockCalls;
    int unlockCalls;
    int blitCalls;
    int getErrorCalls;
    int wasInitCalls;
    int initCalls;
    int openFontCalls;
    int closeFontCalls;
    int setFontSizeCalls;
    int glyphMetricsCalls;
    int renderGlyphCalls;
    int textureCreateCalls;
    int textureDestroyCalls;
    int textureUpdateCalls;
    int debugNameCalls;
    int lockResult;
    unsigned createFlags;
    int createWidth;
    int createHeight;
    int createDepth;
    unsigned createFormat;
    int lastPointSize;
    unsigned short lastCharacter;
    unsigned lastColor;
    SDL_Rect sourceRect;
    SDL_Rect destinationRect;
    void *texturePixels;
    std::uint64_t textureSize;
};

static AtlasTrace trace_state;
static SDL_Surface atlas_surface;
static SDL_Surface glyph_surface;
static unsigned char atlas_pixels[32 * 16 * 4];
static int font_token;

class TestTexture final : public PHL::Texture2D {
public:
    TestTexture(
        std::uint64_t width,
        std::uint64_t height,
        PHL::TextureFormat format)
        : Texture2D(width, height, format)
    {
    }

    ~TestTexture() override
    {
        ++trace_state.textureDestroyCalls;
    }

    void UpdateTexture(void *data, std::uint64_t size) override
    {
        ++trace_state.textureUpdateCalls;
        trace_state.texturePixels = data;
        trace_state.textureSize = size;
    }

    void *GetNativeResource() override
    {
        return nullptr;
    }

    void SetDebugName(const char *name) override
    {
        CHECK_NO_RETURN(name != nullptr);
        CHECK_NO_RETURN(std::strcmp(name, "FontAtlas") == 0);
        ++trace_state.debugNameCalls;
    }

private:
    static void CHECK_NO_RETURN(bool condition)
    {
        if (!condition) {
            std::abort();
        }
    }
};

namespace PHL {

Texture2D::~Texture2D() = default;

Texture2D *Texture2D::CreateTexture(
    std::uint64_t width,
    std::uint64_t height,
    TextureFormat format)
{
    ++trace_state.textureCreateCalls;
    if (width != 32 || height != 16 ||
        format != TextureFormat::RGBA8888) {
        std::abort();
    }
    return new TestTexture(width, height, format);
}

} // namespace PHL

static SDL_Surface *create_surface(
    std::uint32_t flags,
    int width,
    int height,
    int depth,
    std::uint32_t format)
{
    ++trace_state.createSurfaceCalls;
    trace_state.createFlags = flags;
    trace_state.createWidth = width;
    trace_state.createHeight = height;
    trace_state.createDepth = depth;
    trace_state.createFormat = format;
    std::memset(&atlas_surface, 0, sizeof(atlas_surface));
    atlas_surface.w = width;
    atlas_surface.h = height;
    atlas_surface.pitch = width * 4;
    atlas_surface.pixels = atlas_pixels;
    return &atlas_surface;
}

static void free_surface(SDL_Surface *surface)
{
    if (surface == &atlas_surface) {
        ++trace_state.freeAtlasCalls;
    } else if (surface == &glyph_surface) {
        ++trace_state.freeGlyphCalls;
    }
}

static int lock_surface(SDL_Surface *surface)
{
    if (surface != &atlas_surface) {
        std::abort();
    }
    ++trace_state.lockCalls;
    return trace_state.lockResult;
}

static void unlock_surface(SDL_Surface *surface)
{
    if (surface != &atlas_surface) {
        std::abort();
    }
    ++trace_state.unlockCalls;
}

static int upper_blit(
    SDL_Surface *source,
    const SDL_Rect *source_rect,
    SDL_Surface *destination,
    SDL_Rect *destination_rect)
{
    if (source != &glyph_surface || destination != &atlas_surface) {
        std::abort();
    }
    ++trace_state.blitCalls;
    trace_state.sourceRect = *source_rect;
    trace_state.destinationRect = *destination_rect;
    return 0;
}

static const char *get_error()
{
    ++trace_state.getErrorCalls;
    return "test SDL error";
}

static int ttf_was_init()
{
    ++trace_state.wasInitCalls;
    return 0;
}

static int ttf_init()
{
    ++trace_state.initCalls;
    return 0;
}

static _TTF_Font *open_font(const char *file_name, int point_size)
{
    if (std::strcmp(file_name, "regular.ttf") != 0 || point_size != 0) {
        std::abort();
    }
    ++trace_state.openFontCalls;
    return reinterpret_cast<_TTF_Font *>(&font_token);
}

static void close_font(_TTF_Font *font)
{
    if (font != reinterpret_cast<_TTF_Font *>(&font_token)) {
        std::abort();
    }
    ++trace_state.closeFontCalls;
}

static int set_font_size(_TTF_Font *font, int point_size)
{
    if (font != reinterpret_cast<_TTF_Font *>(&font_token)) {
        std::abort();
    }
    ++trace_state.setFontSizeCalls;
    trace_state.lastPointSize = point_size;
    return 0;
}

static int glyph_metrics(
    _TTF_Font *font,
    unsigned short character,
    int *minimum_x,
    int *maximum_x,
    int *minimum_y,
    int *maximum_y,
    int *advance)
{
    if (font != reinterpret_cast<_TTF_Font *>(&font_token)) {
        std::abort();
    }
    ++trace_state.glyphMetricsCalls;
    if (character == 'Z') {
        *minimum_x = 0;
        *maximum_x = 40;
        *minimum_y = 0;
        *maximum_y = 4;
        *advance = 40;
    } else if (character == 'A') {
        *minimum_x = -1;
        *maximum_x = 4;
        *minimum_y = -2;
        *maximum_y = 8;
        *advance = 6;
    } else {
        *minimum_x = 0;
        *maximum_x = 2;
        *minimum_y = 0;
        *maximum_y = 4;
        *advance = 99;
    }
    return 0;
}

static SDL_Surface *render_glyph(
    _TTF_Font *font,
    unsigned short character,
    std::uint32_t color)
{
    if (font != reinterpret_cast<_TTF_Font *>(&font_token)) {
        std::abort();
    }
    ++trace_state.renderGlyphCalls;
    trace_state.lastCharacter = character;
    trace_state.lastColor = color;
    return &glyph_surface;
}

static int font_ascent(_TTF_Font *font)
{
    if (font != reinterpret_cast<_TTF_Font *>(&font_token)) {
        std::abort();
    }
    return 12;
}

static int check_rect(
    const SDL_Rect &rect,
    int x,
    int y,
    int width,
    int height)
{
    CHECK(rect.x == x);
    CHECK(rect.y == y);
    CHECK(rect.w == width);
    CHECK(rect.h == height);
    return 0;
}

int main()
{
    const JPBFontAtlasPlatformHooks hooks = {
        create_surface,
        free_surface,
        lock_surface,
        unlock_surface,
        upper_blit,
        get_error,
        ttf_was_init,
        ttf_init,
        open_font,
        close_font,
        set_font_size,
        glyph_metrics,
        render_glyph,
        font_ascent};

    std::memset(&trace_state, 0, sizeof(trace_state));
    std::memset(&glyph_surface, 0, sizeof(glyph_surface));
    jpb_FontAtlasSetPlatformHooks(&hooks);

    {
        FontAtlas atlas(0x12345678U, 32, 16);
        CHECK(trace_state.createSurfaceCalls == 1);
        CHECK(trace_state.createFlags == 0);
        CHECK(trace_state.createWidth == 32);
        CHECK(trace_state.createHeight == 16);
        CHECK(trace_state.createDepth == 32);
        CHECK(trace_state.createFormat == 0x12345678U);
        CHECK(trace_state.textureCreateCalls == 1);
        CHECK(trace_state.debugNameCalls == 1);
        CHECK(trace_state.wasInitCalls == 1);
        CHECK(trace_state.initCalls == 1);
        CHECK(atlas.GetWidth() == 32);
        CHECK(atlas.GetHeight() == 16);
        CHECK(atlas.GetAtlasSurface() == &atlas_surface);
        CHECK(atlas.GetTexture() != nullptr);
        CHECK(!atlas.IsDirty());

        auto first = atlas.AddGlyph("regular.ttf", 'A', 24);
        CHECK(first.has_value());
        CHECK(first->first.FontFile == "regular.ttf");
        CHECK(first->first.Character == 'A');
        CHECK(first->first.FontSize == 24);
        CHECK(first->first.Metrics.minx == -1);
        CHECK(first->first.Metrics.maxx == 4);
        CHECK(first->first.Metrics.miny == -2);
        CHECK(first->first.Metrics.maxy == 8);
        CHECK(first->first.Metrics.advance == 6);
        CHECK(first->second.Left == 0);
        CHECK(first->second.Top == 0);
        CHECK(first->second.Right == 5);
        CHECK(first->second.Bottom == 10);
        CHECK(trace_state.openFontCalls == 1);
        CHECK(trace_state.setFontSizeCalls == 2);
        CHECK(trace_state.glyphMetricsCalls == 1);
        CHECK(trace_state.renderGlyphCalls == 1);
        CHECK(trace_state.blitCalls == 1);
        CHECK(trace_state.lastCharacter == 'A');
        CHECK(trace_state.lastColor == UINT32_C(0xffffffff));
        CHECK(check_rect(trace_state.sourceRect, 0, 4, 5, 10) == 0);
        CHECK(check_rect(trace_state.destinationRect, 0, 0, 5, 10) == 0);
        CHECK(trace_state.freeGlyphCalls == 1);
        CHECK(atlas.IsDirty());

        Glyph lookup = {
            "regular.ttf", 'A', 24, {99, 98, 97, 96, 95}};
        CHECK(atlas.Contains(lookup));
        auto lookup_entry = atlas.GetGlyphEntry(lookup);
        CHECK(lookup_entry.has_value());
        CHECK(lookup_entry->Right == 5);

        auto cached = atlas.AddGlyph("regular.ttf", 'A', 24);
        CHECK(cached.has_value());
        CHECK(trace_state.openFontCalls == 1);
        CHECK(trace_state.glyphMetricsCalls == 1);
        CHECK(trace_state.renderGlyphCalls == 1);

        atlas.ClearAtlas();
        CHECK(atlas.Contains(lookup));

        atlas.UpdateTexture();
        CHECK(trace_state.lockCalls == 1);
        CHECK(trace_state.textureUpdateCalls == 1);
        CHECK(trace_state.texturePixels == atlas_pixels);
        CHECK(trace_state.textureSize == sizeof(atlas_pixels));
        CHECK(trace_state.unlockCalls == 1);
        CHECK(!atlas.IsDirty());

        atlas.UpdateTexture();
        CHECK(trace_state.textureUpdateCalls == 2);
        CHECK(trace_state.unlockCalls == 2);

        auto apostrophe = atlas.AddGlyph("regular.ttf", 0x2019U, 24);
        CHECK(apostrophe.has_value());
        CHECK(apostrophe->first.Metrics.advance == 10);
        CHECK(apostrophe->second.Left == 5);
        CHECK(apostrophe->second.Right == 7);

        auto quotation = atlas.AddGlyph("regular.ttf", 0x201DU, 24);
        CHECK(quotation.has_value());
        CHECK(quotation->first.Metrics.advance == 18);
        CHECK(quotation->second.Left == 7);
        CHECK(quotation->second.Right == 9);

        auto oversized = atlas.AddGlyph("regular.ttf", 'Z', 24);
        CHECK(!oversized.has_value());
        CHECK(trace_state.renderGlyphCalls == 3);

        trace_state.lockResult = -1;
        atlas.UpdateTexture();
        CHECK(trace_state.getErrorCalls == 1);
        CHECK(trace_state.textureUpdateCalls == 3);
        CHECK(trace_state.unlockCalls == 3);
    }

    CHECK(trace_state.closeFontCalls == 1);
    CHECK(trace_state.freeAtlasCalls == 1);
    CHECK(trace_state.textureDestroyCalls == 1);
    jpb_FontAtlasSetPlatformHooks(nullptr);
    std::puts("FontAtlas tests passed");
    return 0;
}
