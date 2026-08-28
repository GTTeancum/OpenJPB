#ifndef JPB_GLYPH_PACKING_H
#define JPB_GLYPH_PACKING_H

#include <cstddef>
#include <optional>

struct GlyphMetrics {
    int minx;
    int maxx;
    int miny;
    int maxy;
    int advance;
};

struct GlyphEntry {
    unsigned Left;
    unsigned Top;
    unsigned Right;
    unsigned Bottom;
};

struct vec2u {
    unsigned X;
    unsigned Y;
};

class GlyphPackingPolicy {
public:
    GlyphPackingPolicy(unsigned width, unsigned height)
        : Width(width), Height(height)
    {
    }
    virtual std::optional<GlyphEntry> AddGlyphEntry(
        const GlyphMetrics *metrics) = 0;

protected:
    unsigned Width;
    unsigned Height;
};

struct ShelfGlyphPackingPolicyLayoutVerifier;

class ShelfGlyphPackingPolicy : public GlyphPackingPolicy {
public:
    ShelfGlyphPackingPolicy(unsigned width, unsigned height)
        : GlyphPackingPolicy(width, height), Pos{0, 0}, MaxY(0)
    {
    }

    std::optional<GlyphEntry> AddGlyphEntry(
        const GlyphMetrics *metrics) override;

private:
    friend struct ShelfGlyphPackingPolicyLayoutVerifier;
    vec2u Pos;
    unsigned MaxY;
};

struct ShelfGlyphPackingPolicyLayoutVerifier {
    static constexpr std::size_t PosOffset =
        offsetof(ShelfGlyphPackingPolicy, Pos);
    static constexpr std::size_t MaxYOffset =
        offsetof(ShelfGlyphPackingPolicy, MaxY);
};

static_assert(sizeof(GlyphMetrics) == 20, "GlyphMetrics PDB layout changed");
static_assert(sizeof(GlyphEntry) == 16, "GlyphEntry PDB layout changed");
static_assert(sizeof(GlyphPackingPolicy) == 16,
              "GlyphPackingPolicy PDB layout changed");
static_assert(ShelfGlyphPackingPolicyLayoutVerifier::PosOffset == 16,
              "ShelfGlyphPackingPolicy.Pos PDB offset changed");
static_assert(ShelfGlyphPackingPolicyLayoutVerifier::MaxYOffset == 24,
              "ShelfGlyphPackingPolicy.MaxY PDB offset changed");
static_assert(sizeof(ShelfGlyphPackingPolicy) == 32,
              "ShelfGlyphPackingPolicy PDB layout changed");

#endif
