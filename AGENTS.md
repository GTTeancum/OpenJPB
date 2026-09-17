# Canonical reconstruction sources

- When implementing or correcting canonical behavior in this repository, derive behavior only from authoritative local artifacts such as available PDB symbols, types, and data or direct reverse engineering of the shipped executable.
- Do not use online references, videos, guides, visual approximation, or inferred look-and-feel as implementation evidence for canonical behavior in this repository.

# Live-review deployment

- Maintain both PC and Xbox builds. Keep gameplay shared where possible, isolate Xbox adapters under `xbox/`, and validate relevant changes on both targets. Xbox uses nxdk, never the official XDK.

- Name future Windows game builds and deployed executables `OpenJPB.exe`. The CMake target remains `jpb_pc_game`.
- Deploy every test-ready build to the game folder by default before marking an item ready for live review.
- If deployment is blocked because the user is actively testing or the executable is locked, report deployment as pending and complete it as soon as the game process exits.

# Repository hygiene

- Keep generated Xbox test ISOs, XEMU captures, and temporary profiling artifacts out of the source repository. Reuse a single ISO per test location and clean disposable outputs after each run; never accumulate timestamped full-game copies in `xbox/build`.
- Monitor free disk space before and after large Xbox tests. Preserve source assets, the staged game copy, current build products, and evidence needed for review; remove only verified disposable artifacts.
