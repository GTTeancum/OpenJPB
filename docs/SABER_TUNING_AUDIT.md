# Saber Tuning Audit

Updated: 2026-08-28

## Status

PENDING USER REVIEW - the complete saber presentation path is reconstructed from the shipped executable: static glow/core, attack ribbon, Blade Extender, Blade Amplifier glow/cylinders, immediate-polygon submission, culling, transparency, depth, texture, and pose publication. The old host-authored substitutions are removed. The 2026-08-28 Maul attachment correction is implemented and internally verified, but has not yet received live user sign-off.

## Double-Blade Audit Correction

- The blanket audit missed the visible Maul defect. It exercised playable Maul (`model 5`) only, omitted Core Maul (`model 43`), and used a validator that repeated the renderer's endpoint formula. That circular check accepted the detached segment.
- Direct executable disassembly confirms the legacy Maul constants pair nodes `0x12/0x13` and `0x17/0x13`. Both installed `maul_p.bmd` and `maul_d.bmd` instead author two independent blade chains: `v_weapon2` (`0x12`) to `v_coll1` (`0x14`), and `v_weapon3` (`0x13`) to `v_coll4` (`0x17`). Applying the legacy pairing to those shipped nodes deterministically reproduces the offset shown in the README capture.
- The live path now follows those two authoritative BMD chains. Both blades use the normal fixed-point `0x70` extent from their respective hilt attachment node. Endpoint arithmetic also now preserves the executable's signed low-word behavior at world-coordinate wrap boundaries.
- Runtime validation no longer relies solely on duplicated endpoint math. For either Maul model it first identifies all four loaded nodes by their shipped geometry names, then requires two matching colored/core draw pairs whose inner endpoints are the two hilt attachment nodes.
- Focused unit coverage includes both model IDs and a signed-low-word boundary regression. Real-asset 1920x1080 runs pass for playable Maul in Marsh and Core Maul in Core with `outer=2`, `core=2`, `attached=2`, `unmatched=0`, and blade lengths `111.8..112.2`.
- Full-resolution proof and post-capture crops are retained at `C:\Users\smmel\AppData\Local\Temp\openjpb-maul5-attachment.png`, `C:\Users\smmel\AppData\Local\Temp\openjpb-maul5-attachment-crop.png`, `C:\Users\smmel\AppData\Local\Temp\openjpb-maul43-attachment.png`, and `C:\Users\smmel\AppData\Local\Temp\openjpb-maul43-attachment-crop.png`.

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
- `D3DTransparencyPass::CreatePipelineState` at RVA `0x41850` installs `DepthEnable=1`, `DepthWriteMask=ZERO`, and `DepthFunc=LESS_EQUAL` from the descriptor literal at RVA `0x335640`. The second transparency PSO copies the same depth descriptor and changes only its blend state. The port's former strict `LESS` comparison was not canonical and could reject glow/core pixels at equal scene depth.
- `NoScaleEndPoly` at RVA `0x119280` performs its backface gate in projected NDC and converts the signed area with `CVTTSS2SI`. Tiny negative areas truncate to zero and remain visible. The port incorrectly repeated this gate later in framebuffer-pixel coordinates with a strict floating-point `< 0` check, so the same cap could be accepted at one angle/resolution and disappear at the next. The later pixel-space gate is now disabled for no-scale polygons and an exact level-2 cap regression locks the shipped behavior.
- `jedi_HandleSabre` supplies a normal extent of `112`, outer width `rand()%6 + 14` (`14..19`), and white-core width `2`.
- Blade Extender supplies extent `196`, outer width `(rand()&7) + 24` (`24..31`), and white-core width `6`. Blade Amplifier's extra glow width is `16`.
- `jedi_DrawBlur` at RVA `0xB1890` now emits its recovered four-point `whitematAdd` motion ribbon. Tip history is clamped to `0x60`, hilt history uses one quarter of that tip extent, all four points are transformed by `fRotTransPers`, and the quad is submitted through `_NoScaleEndPoly` with the exact color bit.
- `drawCylinder` and `drawCylinderG` at RVAs `0xF84B0` and `0xF87E0` now emit their 16 authored quads from 34 transformed ring vertices. The runtime no longer turns powered-saber cylinders into extra `fx_screenGlow` segments.
- Model rendering publishes each attachment node's current world center and per-frame velocity immediately before `player_HandleSabre`, matching the shipped `render_RenderNode` ordering.
- The diagnostic glow hook remains observation-only. The removed D3D procedural line shader and software glow-disc renderer can no longer substitute for or double-render the canonical geometry.

## Existing Color Selection

- Legacy/original saber color table remains unchanged.
- Remaster/current colors now use:
  - Blue: packed RGB `0x45a6ff` for Obi-Wan, Adi Gallia, Plo Koon, and Ki-Adi-Mundi in the current/canon table.
  - Mace Windu canon purple: packed RGB `0xd870ff`.
  - Darth Maul red: packed RGB `0xff3434`.

These table values predate the blade-renderer reconstruction. They select the packed color passed into the exact renderer and remain independently regression-covered.

## Proof Artifacts

- Native D3D11 1920x1080 level-2 frames `2..8` were captured and inspected sequentially before cropping in an unobstructed open area. Each full frame retains a continuous white core, connected blue halo, and complete tip through the near-end-on and side angles that previously lost cap quads. The review crop is `C:\Users\smmel\AppData\Local\Temp\openjpb-saber-marsh-full-re\d3d-frames-2-8-crop.png`; it is intentionally outside the repository to avoid adding capture bulk.
- A second hidden-D3D level-2 sequence drives the ordinary attack input and captures frames `10`, `12`, `14`, and `16`; each full-resolution frame was inspected before the review crop at `C:\Users\smmel\AppData\Local\Temp\openjpb-saber-marsh-full-re\d3d-attack-frames-10-16-crop.png` was produced.
- Runtime diagnostics remain valid implementation evidence: every frame `2..8` passes `--validate-player-saber`; frame 8 reports `saber_glow=2/0` and `screen_poly=13/0/26`. The first 12 polygons are the outer/core `a_glow.tga` quads, all report `dropped=0`, and the thirteenth is the player shadow.

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

- Build: `cmake --build build --config Release --target jpb_projection_tests jpb_menu_tests jpb_player_tests jpb_pc_game`
- PASS: focused projection, menu, and player suites, plus PC game, versus, authored-camera, actor-class, pickup-contact, and death-restart smoke reached during the broader registry run.
- PASS: the projection test checks all 24 emitted vertices across all six quads, including exact topology, UV selection, packed color, no-scale mode, and depth bias. Runtime traces prove all 12 blade polygons use installed `a_glow.tga`.
- PASS: the projection test requires an equal-depth transparency card to draw and an opaque card at the same depth to fail, covering the executable's `LESS_EQUAL`/`LESS` distinction.
- PASS: projection tests require the exact level-2 tiny-negative cap to survive the shipped NDC integer gate, require the recovered blur's four transformed points/material/color, and require all 16 cylinder quads with authored UV ordering.
- PASS: staged 1920x1080 FED software smoke with `--validate-player-saber`: normal extent `112.4`, outer width `19`, one colored pass, one white-core pass, 12 canonical glow polygons, and zero dropped glow/polygon draws.
- PASS: staged hidden D3D FED smoke: hardware backend active, normal extent `112.4`, outer width `19`, one colored pass, one white-core pass, 12 canonical glow polygons, and zero dropped glow/polygon draws.
- PASS: staged hidden D3D level-2 frames `2..8` at 1920x1080: hardware backend active on the AMD Radeon 780M, normal extent `111.8`, outer width `15`, one colored pass, one white-core pass, 12 canonical glow polygons, and zero dropped glow/polygon draws on every validated run.
- PASS: staged hidden D3D level-2 attack frames `10`, `12`, `14`, and `16` at 1920x1080: every run passes saber validation and visual inspection across the changing blade orientation.
- PASS: targeted `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Mace, Adi, Plo, Maul, and Ki-Adi with `--validate-player-saber --framebuffer-size 1920 1080`; each console log records `framebuffer=1920x1080` and `source=1920x1080`.
- PASS: targeted HD `jpb_pc_game.exe` runtime saber diagnostics for Adi and Plo with `--player-saber-color legacy`; logs record Adi `player_weapon color=7fc02010` and Plo `player_weapon color=7ff8c001`.
- PASS: focused Plo side-profile proof uses the west attack at frame 16 from a 1920x1080 source frame; the log records `player_weapon=(model=4,saber=1,color=7ff8c001,...,core=1,attached=1,...)`.
- Registry limitation: the RelWithDebInfo CTest registry contains 861 entries, many targeting binaries not built by the focused configuration. Those entries report `Not Run`; the run was stopped after test 507 once all reached gameplay smoke tests had passed.
