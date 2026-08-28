/*
 * COMPLETE REVIEWED RECONSTRUCTION of the project-owned factory boundary.
 * PDB module: 0070
 * Object: W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\Texture2D.obj
 * Primary source: W:\SWJediPowerBattles\work\rendering\Texture2D.cpp
 * Compiler language: c++
 * Emitted procedures: 2
 *
 * Direct shipped-executable disassembly at RVA 0xEE130 confirms the exact
 * 0x1A0-byte allocation, Texture constructor handoff, null return, and local
 * unwind boundary. The real D3D12 construction path is covered by the
 * d3dtextr focused suite in Debug and Release.
 */

#include "jpb/texture2d.h"
#include "jpb/d3dtextr.h"

/* 0xEE130, 96 bytes, global, 3 named locals
 * PHL::Texture2D::CreateTexture
 * PDB type: PHL::Texture2D* PHL::Texture2D::...
 * Source: W:\SWJediPowerBattles\work\rendering\Texture2D.cpp
 */
PHL::Texture2D *PHL::Texture2D::CreateTexture(
    std::uint64_t width,
    std::uint64_t height,
    PHL::TextureFormat format)
{
    return new Texture(width, height, format);
}

/* 0x270670, 29 bytes, local, 0 named locals
 * `PHL::Texture2D::CreateTexture'::`1'::dtor$0
 * PDB type: unknown
 * Source: no line mapping
 */
