# Saber Tuning Audit

Updated: 2026-08-15

## Status

PASS - first remaster-reference tuning pass is implemented and validated through the focused menu color-toggle test plus headless player saber runtime diagnostics. The review proofs were captured at 1920x1080 before cropping, including the current/canon colors and Adi/Plo classic alt colors.

## Reference Basis

- Official remaster color behavior:
  - PlayStation Blog: `https://blog.playstation.com/2024/12/05/star-wars-episode-i-jedi-power-battles-reveals-new-lightsaber-color-toggle-feature/`
  - StarWars.com launch article: `https://www.starwars.com/news/star-wars-episode-i-jedi-power-battles-announce`
- YouTube remaster visual references sampled:
  - Mace Windu gameplay: `https://www.youtube.com/watch?v=O5x3HdMnJzk`
  - Plo Koon gameplay: `https://www.youtube.com/watch?v=Jqq-ddeuwXo`
  - Darth Maul gameplay: `https://www.youtube.com/watch?v=FKK1YfFSQvQ`
  - Remastered playlist opening/title sample: `https://www.youtube.com/playlist?list=PLWoB_QSNfUdDKoTc5JvSWiC3opVjJl1Ia`
  - Adi Gallia remaster selection thumbnail: `https://www.youtube.com/watch?v=k-aVtG30nDk`
  - Ki-Adi-Mundi remaster selection thumbnail: `https://www.youtube.com/watch?v=YKZNvSjMw54`

The sampled YouTube gameplay confirms the remaster can show either classic or toggled saber colors depending on the uploader's selection, so the canonical color mapping follows the official remaster color-toggle sources. The YouTube clips were used for render character: a bright white core, broader additive halo, and hotter red/purple/blue presentation than the old exact PC constants.

## Implemented Tuning

- Legacy/original saber color table remains unchanged.
- Remaster/current colors now use:
  - Blue: packed RGB `0x45a6ff` for Obi-Wan, Adi Gallia, Plo Koon, and Ki-Adi-Mundi in the current/canon table.
  - Mace Windu canon purple: packed RGB `0xd870ff`.
  - Darth Maul red: packed RGB `0xff3434`.
- Normal blade draw tuning:
  - Outer halo width: `16..20` world units.
  - White core width: `3` world units.
- Blade Extender tuning:
  - Outer halo width: `28..35` world units.
  - White core width: `8` world units.
- Blade Amplifier extra glow width: `18` world units.

## Proof Artifacts

- Current tuned saber capture contact sheets:
  - HD source frames/crops: `out/saber-current-hd/contact-sheet.png`, `out/saber-current-hd/contact-sheet-crop.png`
  - Earlier 960x540 baseline: `out/saber-current/contact-sheet.png`, `out/saber-current/contact-sheet-zoom.png`
- Classic alt saber capture contact sheets:
  - HD source frames/crops: `out/saber-alt-hd/contact-sheet.png`, `out/saber-alt-hd/contact-sheet-crop.png`
  - Adi Gallia legacy alt red: `out/saber-alt-hd/adi.console.txt`, `out/saber-alt-hd/adi.png`, `out/saber-alt-hd/adi.crop.png`
  - Plo Koon legacy alt yellow/orange: `out/saber-alt-hd/plo.console.txt`, `out/saber-alt-hd/plo.png`, `out/saber-alt-hd/plo.crop.png`
  - Plo Koon side-profile core proof: `out/saber-alt-hd/plo-side-profile-proof.png`, `out/saber-alt-hd/plo-side-profile-source.png`, `out/saber-alt-hd/plo-side-profile.console.txt`
- Per-character diagnostics and captures:
  - `out/saber-current-hd/mace.console.txt`, `out/saber-current-hd/mace.png`, `out/saber-current-hd/mace.crop.png`
  - `out/saber-current-hd/adi.console.txt`, `out/saber-current-hd/adi.png`, `out/saber-current-hd/adi.crop.png`
  - `out/saber-current-hd/plo.console.txt`, `out/saber-current-hd/plo.png`, `out/saber-current-hd/plo.crop.png`
  - `out/saber-current-hd/maul_p.console.txt`, `out/saber-current-hd/maul_p.png`, `out/saber-current-hd/maul_p.crop.png`
  - `out/saber-current-hd/ki_adi.console.txt`, `out/saber-current-hd/ki_adi.png`, `out/saber-current-hd/ki_adi.crop.png`
- Reference artifacts retained compactly under `out/saber-reference`; raw MP4 downloads and extracted frame directories were pruned after contact sheets/thumbnails were generated. The large PNG contact sheets were compressed to JPGs to keep the proof footprint small.
Raw HD `.ppm` captures were pruned after PNG conversion.

## Verification

- Build: `cmake --build build --config Release --target jpb_player_tests jpb_menu_tests jpb_pc_game`
- PASS: `ctest --test-dir build -C Release -R "jpb_menu_tests" --output-on-failure`
- PASS: targeted `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber --framebuffer-size 1920 1080`; each console log records `framebuffer=1920x1080` and `source=1920x1080`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Adi and Plo with `--player-saber-color legacy`; logs record Adi `player_weapon color=7fc02010` and Plo `player_weapon color=7ff8c001`.
- PASS: focused Plo side-profile proof uses the west attack at frame 16 from a 1920x1080 source frame; the log records `player_weapon=(model=4,saber=1,color=7ff8c001,...,core=1,attached=1,...)`.
- Known unrelated failure: `jpb_player_tests.exe` still fails in `test_player_damage_tracker_owner` at `tests/test_player.c:1964`; no saber assertions fail before that point.
