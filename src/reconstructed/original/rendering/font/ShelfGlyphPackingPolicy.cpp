/*
 * REVIEWED RECONSTRUCTION of
 * W:\SWJediPowerBattles\work\rendering\font\ShelfGlyphPackingPolicy.cpp.
 * PDB module: 0068
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\ShelfGlyphPackingPolicy.obj
 * Primary source: W:\SWJediPowerBattles\work\rendering\font\ShelfGlyphPackingPolicy.cpp
 * Compiler language: c++
 * Emitted procedures: 1
 *
 * Use inventory/function_map.tsv with ExportReconstruction.java.
 */

#include "jpb/glyph_packing.h"

/* 0xEDCF0, 203 bytes, global, 6 named locals
 * ShelfGlyphPackingPolicy::AddGlyphEntry
 * PDB type: std::optional<GlyphEntry> ShelfG...
 * Source: W:\SWJediPowerBattles\work\rendering\font\ShelfGlyphPackingPolicy.cpp
 */
std::optional<GlyphEntry> ShelfGlyphPackingPolicy::AddGlyphEntry(
    const GlyphMetrics *metrics)
{
    unsigned glyphWidth = (unsigned)(metrics->maxx - metrics->minx);
    unsigned glyphHeight = (unsigned)(metrics->maxy - metrics->miny);
    unsigned left = Pos.X;
    unsigned top = Pos.Y;

    if (Pos.Y + glyphHeight < Height) {
        if (Width <= Pos.X + glyphWidth) {
            left = 0;
            Pos.Y += MaxY;
            top = Pos.Y;
            Pos.X = 0;
            MaxY = 0;
        }

        Pos.X = left + glyphWidth;
        if (Pos.X < Width && Pos.Y < Height) {
            GlyphEntry entry = {
                left,
                top,
                Pos.X,
                Pos.Y + glyphHeight
            };

            if (MaxY < glyphHeight) {
                MaxY = glyphHeight;
            }
            return entry;
        }
    }
    return std::nullopt;
}
