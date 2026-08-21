# Saber Tuning Audit

Updated: 2026-08-21

## Status

PENDING USER REVIEW - the static blade renderer and its caller constants are reconstructed from the shipped executable. The old host-authored line/disc glow paths have been removed; software and D3D now consume the same six canonical `a_glow.tga` quads.

## Visual Context Only

The user-provided remaster screenshot from 2026-08-21 and the historical links below are reference-only visual context. They are not authoritative implementation evidence. All future blade implementation decisions must come from the local PDB, shipped executable, and installed data.

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

The links and captures record what informed the earlier visual tuning pass. That pass is retained as a provisional baseline only and must not be treated as recovered remaster behavior.

## Recovered Blade Path

- Direct shipped-EXE disassembly of `fx_screenGlow` at RVA `0xA3C40` recovers the complete 1,114-byte function.
- `PerspectiveTransform(&CameraMatrix, ...)` produces the two camera-space endpoints. The perpendicular offsets are normalized from the camera-space X/Y delta and multiplied directly by the caller's width; there is no host viewport-distance scaling rule.
- Both endpoint depths receive the exact `1.6e-6` bias before float storage.
- The routine builds 12 vertices and emits six `_StartPoly(4, additive_glowtexture)` / `_SetVert` / `_NoScaleEndPoly` quads.
- The executable topology words are `54103210`, `98541032`, `65213311`, `a9651133`, `76322301`, and `ba760123`. The UV table is exactly `(0.01,0.01)`, `(0.99,0.01)`, `(0.01,0.99)`, `(0.99,0.99)`.
- The installed `a_glow.tga` is the texture used by the recovered path. Packed blade color and alpha pass through unchanged.
- `jedi_HandleSabre` supplies a normal extent of `112`, outer width `rand()%6 + 14` (`14..19`), and white-core width `2`.
- Blade Extender supplies extent `196`, outer width `(rand()&7) + 24` (`24..31`), and white-core width `6`. Blade Amplifier's extra glow width is `16`.
- The diagnostic glow hook remains observation-only. The removed D3D procedural line shader and software glow-disc renderer can no longer substitute for or double-render the canonical geometry.
- Scope: this closes the visible static blade. Attack blur/trails and powered-cylinder flourishes are separate effects and are not represented as recovered by this audit.

## Existing Color Selection

- Legacy/original saber color table remains unchanged.
- Remaster/current colors now use:
  - Blue: packed RGB `0x45a6ff` for Obi-Wan, Adi Gallia, Plo Koon, and Ki-Adi-Mundi in the current/canon table.
  - Mace Windu canon purple: packed RGB `0xd870ff`.
  - Darth Maul red: packed RGB `0xff3434`.

These table values predate the blade-renderer reconstruction. They select the packed color passed into the exact renderer and remain independently regression-covered.

## Proof Artifacts

- Pending: replace the rejected FED frame with a close, unobstructed native-1080 capture that clearly shows the white core, colored halo, width, and tip geometry. The rejected frame is not accepted as visual proof.
- Runtime diagnostics remain valid implementation evidence: `saber_glow=2/0`, `screen_poly=13/0/1874`; the first 12 polygons are the outer/core `a_glow.tga` quads and the thirteenth is the player shadow.

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

- Build: `cmake --build build --config Release --target jpb_projection_tests jpb_pc_game`
- PASS: focused `force_activate`, `force`, `projection`, `player`, `enemy`, and PC game smoke tests (`6/6`).
- PASS: the projection test checks all 24 emitted vertices across all six quads, including exact topology, UV selection, packed color, no-scale mode, and depth bias. Runtime traces prove all 12 blade polygons use installed `a_glow.tga`.
- PASS: staged 1920x1080 FED software smoke with `--validate-player-saber`: normal extent `112.4`, outer width `19`, one colored pass, one white-core pass, 12 canonical glow polygons, and zero dropped glow/polygon draws.
- PASS: staged hidden D3D FED smoke: hardware backend active, normal extent `112.4`, outer width `19`, one colored pass, one white-core pass, 12 canonical glow polygons, and zero dropped glow/polygon draws.
- PASS: targeted `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber --framebuffer-size 1920 1080`; each console log records `framebuffer=1920x1080` and `source=1920x1080`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Adi and Plo with `--player-saber-color legacy`; logs record Adi `player_weapon color=7fc02010` and Plo `player_weapon color=7ff8c001`.
- PASS: focused Plo side-profile proof uses the west attack at frame 16 from a 1920x1080 source frame; the log records `player_weapon=(model=4,saber=1,color=7ff8c001,...,core=1,attached=1,...)`.
- Known unrelated full-suite failure: `jpb_bmd_tests` currently fails its existing `render_stats.modelPixels > 0` assertion.
