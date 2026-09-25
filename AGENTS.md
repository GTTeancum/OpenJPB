# Canonical reconstruction sources

- When implementing or correcting canonical behavior in this repository, derive behavior only from authoritative local artifacts such as available PDB symbols, types, and data or direct reverse engineering of the shipped executable.
- Do not use online references, videos, guides, visual approximation, or inferred look-and-feel as implementation evidence for canonical behavior in this repository.

# Live-review deployment

- Maintain both PC and Xbox builds. Keep gameplay shared where possible, isolate Xbox adapters under `xbox/`, and validate relevant changes on both targets. Xbox uses nxdk, never the official XDK.

- Name future Windows game builds and deployed executables `OpenJPB.exe`. The CMake target remains `jpb_pc_game`.
- Deploy every test-ready build to the game folder by default before marking an item ready for live review.
- If deployment is blocked because the user is actively testing or the executable is locked, report deployment as pending and complete it as soon as the game process exits.

# Repository hygiene

- Keep Xbox test ISOs, XEMU captures, telemetry, and temporary profiling artifacts under the ignored repository-local `xbox/test-artifacts/` directory. Reuse its single `OpenJPB-current.iso` and clean disposable outputs after each run; never place test artifacts in external game or ISO folders.
- The external `C:/Games/OpenJPB-Xbox` folder is release deployment only: keep `default.xbe` and `res/` there. Store the asset manifest under `xbox/build/`, and test control markers under `xbox/test-config/active/`; the XEMU launcher may copy markers into the release tree only while constructing a test ISO and must remove them before it exits.
- Monitor free disk space before and after large Xbox tests. Preserve source assets, the staged game copy, current build products, and evidence needed for review; remove only verified disposable artifacts.
