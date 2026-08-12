# Third-party source

## stb_truetype

`stb/stb_truetype.h` is stb_truetype 1.26 from the official
[`nothings/stb`](https://github.com/nothings/stb) repository, pinned to commit
`2c980bb59875b0d32144a71867fbdebb2f77cd20`.

- Upstream file: `stb_truetype.h`
- SHA-256: `ecd30b05e0dd4fea3a13c26810dd9e1992dc379049482c393d5a19e6b5090aab`
- License choice for this repository: MIT (the full dual MIT/public-domain
  notice is retained at the end of the vendored header)

The reconstruction uses this single-header source as a platform-neutral
TrueType rasterizer. It does not introduce an external runtime library. Font
files remain original-game assets and are not copied into this source tree.

## ufbx

The Win32 level-import adapter and FBX evidence probes require the exact ufbx
0.6.1 source (`UFBX_HEADER_VERSION == 6001`) through the CMake cache path
`JPB_UFBX_SOURCE_DIR`. The source is not currently vendored. ufbx is confined
to `src/port/pc_level_fbx.c`; it is not included by the portable runtime or
software renderer, which consume flattened `JPBSoftwareLevelMesh` records.
