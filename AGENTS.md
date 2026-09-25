# Canonical reconstruction sources

- When implementing or correcting canonical behavior in this repository, derive behavior only from authoritative local artifacts such as available PDB symbols, types, and data or direct reverse engineering of the shipped executable.
- Do not use online references, videos, guides, visual approximation, or inferred look-and-feel as implementation evidence for canonical behavior in this repository.

# Live-review deployment

- Maintain both PC and Xbox builds. Keep gameplay shared where possible, isolate Xbox adapters under `xbox/`, and validate relevant changes on both targets. Xbox uses nxdk, never the official XDK.

- Name future Windows game builds and deployed executables `OpenJPB.exe`. The CMake target remains `jpb_pc_game`.
- Deploy every test-ready PC build to the installed PC game folder by default before marking an item ready for live review. Keep all Xbox executables, staged assets, test ISOs, captures, and diagnostics inside the repository; the user retrieves Xbox release files from `xbox/build/`.
- If deployment is blocked because the user is actively testing or the executable is locked, report deployment as pending and complete it as soon as the game process exits.

# Repository hygiene

- Keep Xbox test ISOs, XEMU captures, telemetry, and temporary profiling artifacts under the ignored repository-local `xbox/test-artifacts/` directory. Reuse its single `OpenJPB-current.iso` and clean disposable outputs after each run; never place test artifacts in external game or ISO folders.
- Keep the staged Xbox disc under `xbox/build/staged-disc/`, the release XBE under `xbox/build/release/`, the asset manifest under `xbox/build/`, and test control markers under `xbox/test-config/active/`. The only project artifact deployed outside the repository is the PC `OpenJPB.exe` in the installed PC game folder.
- Monitor free disk space before and after large Xbox tests. Preserve source assets, the staged game copy, current build products, and evidence needed for review; remove only verified disposable artifacts.
