# Original Xbox target

The current project order is complete PC reconstruction first, followed by a
complete original-Xbox port. A dependency-light PC game runtime now
loads original JPX and CAD data and moves an actor with authored motion through
recovered gameplay state; it deliberately keeps framebuffer, file, and raw-pad
seams suitable for Xbox. This page records the already-proven nxdk toolchain
and ABI result. New Xbox builds and Xbox-specific adapters remain deferred
until the portable PC reconstruction is complete; when that phase begins,
nxdk is the target toolchain.

## Toolchain evidence

The first target build was produced with the existing nxdk checkout at
`C:\nxdk`, Git revision:

```text
fb5a9a7a58a431e8d70a9e7da87898059df376c0
```

The checkout already contains its built libraries and `cxbe`. The build uses
MSYS2 in MinGW64 mode and the LLVM installation at
`C:\Program Files\LLVM`.

From PowerShell:

```powershell
$env:MSYSTEM = "MINGW64"
& "C:\msys64\usr\bin\bash.exe" --noprofile --norc `
  .\tools\build_nxdk.sh -j4
```

## Current target

The root `Makefile` links these reviewed reconstruction modules:

- `list.c`
- `timer.c`
- `alloc.c`
- `memory.c`
- reviewed scalar/normalization portions of `fmath.c`
- reviewed distance/identity/length portions of `vectors.c`

`src/port/xbox_main.c` initializes their state and performs a small on-target
foundation check, including fixed-angle sine, vector normalization, and
identity-matrix initialization. It also includes the game, sprite, and world
headers, so nxdk evaluates the 32-bit `optionstruct`, `SCB`, `Sprite`, `Ring`,
`wsl_BAPAI_DEFAULTS`, and `wsl_BAP_PLACEMENT` layout assertions even though
their full modules are not linked into this PC-first bootstrap yet. A
successful build produces:

```text
build/xbox/default.xbe
```

The current XBE is 122,880 bytes with SHA-256
`F35C67F5F130EA1CA6898B399DD1C3F6875E1D12535CF0916424D9FB5E45F529`.
Its intermediate PE was independently
verified as `COFF-i386`, `IMAGE_FILE_MACHINE_I386`, with a 32-bit address
size. This means the target compiler evaluated the Xbox-specific structure
size assertions in the reviewed headers. The successful link also resolves
nxdk's `sqrt`, `sin`, `cos`, and `atan2` implementations used by the recovered
math layer.

The root Makefile scopes `-std=c11` and project warnings only to reconstruction
objects. This leaves nxdk's own HAL sources in their required GNU language
mode during a clean toolchain rebuild.

## Validation status

- Compile: passed.
- Link against nxdk: passed.
- PE-to-XBE conversion: passed.
- Emulator/hardware launch: pending; no local `xemu` executable was found in
  the standard locations checked during this milestone.
- Complete game port: pending.

The current XBE is a bootstrap and layout/link smoke test, not yet a playable
game build.
