#include "jpb/glyph_packing.h"

#include <cstdio>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            std::fprintf(stderr, "check failed at %s:%d: %s\n",            \
                         __FILE__, __LINE__, #condition);                    \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static int check_entry(
    const std::optional<GlyphEntry> &entry,
    unsigned left,
    unsigned top,
    unsigned right,
    unsigned bottom)
{
    CHECK(entry.has_value());
    CHECK(entry->Left == left);
    CHECK(entry->Top == top);
    CHECK(entry->Right == right);
    CHECK(entry->Bottom == bottom);
    return 0;
}

int main()
{
    ShelfGlyphPackingPolicy policy(10, 10);
    const GlyphMetrics threeByFour = {1, 4, -1, 3, 4};
    const GlyphMetrics sixByFour = {-2, 4, 0, 4, 6};
    const GlyphMetrics twoByFour = {0, 2, 0, 4, 2};
    const GlyphMetrics twoBySix = {0, 2, 0, 6, 2};
    ShelfGlyphPackingPolicy exactWidth(3, 10);

    CHECK(check_entry(
              policy.AddGlyphEntry(&threeByFour), 0, 0, 3, 4) == 0);
    CHECK(check_entry(
              policy.AddGlyphEntry(&sixByFour), 3, 0, 9, 4) == 0);
    CHECK(check_entry(
              policy.AddGlyphEntry(&twoByFour), 0, 4, 2, 8) == 0);
    CHECK(!policy.AddGlyphEntry(&twoBySix).has_value());
    CHECK(!exactWidth.AddGlyphEntry(&threeByFour).has_value());

    std::puts("glyph shelf packing tests passed");
    return 0;
}
