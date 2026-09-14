# Mod character fixes — September 13, 2026

## Coleman Trebor load failure

Reproduced the user's first-frame failure after the FED intro with the character-select/level-select/movie path. The runtime returned BMD rendering error -4, and texture loading attempted to interpret already-relocated material pointers as texture names.

Single-player creates a hidden second hierarchy from P1's BMD. P1 used `native_mod_<id>` but the hidden hierarchy used the stock donor name, causing the same geometry to be prepared again. `loader_CreateCharacter` now uses the P1 package registration for that shared single-player hierarchy. It does not assign a mod identity to the inactive player, alter a real P2 selection, or change the separate level-12 model path.

## Default and alternate saber icons

The legacy patcher's `ICON_INDEX_MAP` maps 0x9D–0xA2 to `Lightsaber_New1.png`–`Lightsaber_New6.png`. Those numbers refer to concept-art slots in the unpatched menu table. Native migration had preserved the numbers without those remappings.

Character manifests now support two explicit package-relative `saberIcons` paths. Menu rendering loads them in a separate logical range, leaving stock concept-art slots intact. Migration copies every referenced default/alternate icon; the repeated migration verifies 26 package image copies against their originals (nine unique source images). Original assets and the legacy executable are unchanged. If a legacy current icon is stale, the current blade color chooses the icon; equal-color variants use the icon value to disambiguate.

## Earlier verification � insufficient for full roster approval

The subsequent live review found donor-model fallback and missing portraits. These earlier seeded-selection smoke runs did not establish normal menu/session correctness. They are superseded by the title-driven audit in [MOD_ROSTER_REVIEW_20260913.md](MOD_ROSTER_REVIEW_20260913.md).

- `jpb_mod_tests` and `jpb_model_tests` pass. New checks cover custom icon paths, toggling, explicit variant selection, stale current icons, missing-file rejection and transactional registry preservation.
- `out/mod-fixes/final-smoke/results.json`: all 27 default/alternate cases pass (13 Jedi with both options, plus Jango without a saber).
- `out/mod-fixes/current-smoke/results.json`: all 14 current-selection cases pass, including the stale legacy pairs, on the final build.
- Each case covers a native menu render, menu-to-FED handoff through the intro movie, and attack/jump inputs followed by walking recovery. Jedi checks require the exact blade color, visible cores, matched attachments and no unmatched attachments. All processes exit cleanly.
- Reviewed all 27 menu captures and paired Coleman/Aayla combat captures. These are smoke checks, not full campaign or combat-balance certification.
- Reusable command: `python tools/smoke_mod_characters.py --game-root <game> --exe <OpenJPB.exe> --output <directory>`; add `--variants current` for saved manifest selections. Tests use hidden windows, process-local input and no user persistence.

Deployed to `C:/Games/Star Wars Jedi Power Battles/OpenJPB.exe` after the previous audio test exited. SHA-256 matches the final tested build: `7E2246223F970024C3C9B01719209B80980F9A477D20BC65E46296F0A9514743`. Launched successfully with live controls and audio as Obi-Wan at FED authored checkpoint record 43 (`fed.pwr`, type 5, position -19925/5352/-6169), on the approach before the second boss; no forced boss activation.
