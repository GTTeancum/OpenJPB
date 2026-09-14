# Title-driven mod roster review — September 13, 2026

## Reopened live failure

The user reported blank portraits after browsing the roster and custom characters appearing as their stock parents. Previous tests seeded the selected model and asserted successful rendering and blade color, not the identity of the final constructed mesh. Their results were insufficient.

## Corrections

- The canonical `loader_LoadJedi` chain rebuilt players from donor BMD/CAD/CMB paths after portable initialization had selected custom files. The three character file loaders now use the selected package when its donor matches the active character; authored special-level animation overrides remain in place.
- The front-end texture adapter had capacity for 249 stock textures. Traversing additions reproduced `menu texture load rejected count=249`; reserve also covers each supported package's portrait and two saber images.
- Title-frame validation now permits a frame that changes menu modes without drawing. Fresh boot can transition into the EULA without being incorrectly treated as a rendering failure.

## Review method

`tools/review_mod_roster.py` starts a hidden interactive renderer with process-local virtual controller input. It copies the user's existing options into an isolated save directory, passes through boot videos and the title menu, opens New Game, navigates the roster, selects a level, skips the FMV, dismisses the level introduction and waits for the authored opening camera sequence. It exercises gameplay, quits through the pause menu, explicitly selects New Game rather than Continue, browses away/back, toggles the saber in the actual menu, and plays the same character again.

No direct quickload, character override, or selector override is used. Original files and live saves remain untouched. Phase captures come from the game's framebuffer, not desktop capture. `--session-review-prefix` is gated on the process-local control harness.

Each gameplay capture compares the constructed BMD's names, IDs, local transforms, face/vertex counts, packed vertex data and UV data against its package file, independent of runtime pointer relocation. Saber checks compare emitted color, cores and attachments against the selected package variant. Both menu selections and both game handoffs are required. Attack frames are recorded when the selected attack or combo actually runs; each gameplay leg must add witnessed attack frames. This avoids incorrectly rejecting an otherwise successful session just because its final state is a later automatic block or hit reaction. Direct headless combat-fixture checks retain their original contract.

## Completed roster verification

All 14 additions pass on build SHA-256 `00E66D22788C6CA74EF71A131D8FB5AC7B43D2A99750741B8F43648A0B48A95D`: 28 normal menu-to-game handoffs, including all 13 Jedi alternate sabers and a second Jango Fett session. Native captures for every character were visually reviewed for portraits, custom models, and saber variants. The geometry comparison additionally distinguishes every constructed model from its donor.

| Character | Portrait / model | Default and alternate saber | Attack in both sessions |
| --- | --- | --- | --- |
| Kit Fisto | Pass | Pass | Pass |
| Aayla Secura | Pass | Pass | Pass |
| Roron Corobb | Pass | Pass | Pass |
| Luminara Unduli | Pass | Pass | Pass |
| Saesee Tiin | Pass | Pass | Pass |
| Tosan | Pass | Pass | Pass |
| Ima-Gun Di | Pass | Pass | Pass |
| Sharad Hett | Pass | Pass | Pass |
| Eeth Koth | Pass | Pass | Pass |
| Coleman Trebor | Pass | Pass | Pass |
| Sora Bulq | Pass | Pass | Pass |
| Jedi Temple Guard | Pass | Pass | Pass |
| Jango Fett | Pass | Not applicable | Pass |
| Shaak Ti | Pass | Pass | Pass |

Machine-readable results: `out/mod-identity/accepted-results.json`. Individual input sequences, commands, logs and framebuffer captures are in `out/mod-identity/verified-roster/<id>/`. Sora Bulq's first run had all three attack presses occur during hit reactions; a repeat with five attempts passed both legs in `out/mod-identity/sora-final/sorabulq/`. The original failed attack check remains preserved. The reusable driver now makes five attempts per leg.

The tested executable has been copied to `C:/Games/Star Wars Jedi Power Battles/OpenJPB.exe` and its hash matches. The final installed-path menu run without `--mods-root` also passes: Coleman Trebor is selected through normal menus twice, both saber variants render, both constructed meshes match the package, attacks execute in both sessions, and exit is clean. Evidence: `out/mod-identity/installed-menu-review/`. Live saves were isolated and legacy assets retained. This verifies the reported roster failures; it does not claim exhaustive completion of every level with every mod.

## Regression scope

All 97 unit tests pass (`ctest -C Release -R '_tests$'`), including the final build's run in `out/mod-identity/unit-tests-final.log`. An exploratory run of the larger 949-entry corpus was stopped after old presentation expectations failed: the FBX summary is not printed where its regex expects it, the selector expects 210 textures rather than the current bank, and an older keyboard ownership case expects a controller press to be ignored. The same mismatches were reproduced with the pre-fix executable (`out/mod-identity/baseline-*.log`). The full 949-entry corpus is not claimed as passing.

The model-copy audit (`out/mod-identity/asset-audit.json`) confirms all 14 package BMDs are byte-identical to their legacy mod source and distinct from their stock animation donor. No legacy asset migration was repeated or altered during this correction.
