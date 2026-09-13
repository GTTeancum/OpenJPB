# Live review — September 12, 2026

## Confirmed closed

The user confirmed the following in the September 11 run ending shortly after midnight:

- Former #1: boss ending reaches results and proceeds into level two.
- Former #2: post-level results presentation is good.
- Former #3: players and enemies can walk off edges correctly.
- Former #4: hidden trigger/script-owner models no longer appear.
- Former #9: title A/B flow and title music work.

Autosave independently confirmed: the installed `SAVEDATA0/Game` was written at September 11, 23:58:22.691 local time. It has the expected 4624-byte payload, version 0, validFlag 1, and lastlevel 2. The live log starts the level-two handoff at 23:58:22.758 and completes Marsh initialization at 23:58:28.237. This is evidence of an actual saved level-two resume point, not merely an enabled AutoSave option. AutoSave is also enabled in Options. No live files were modified during verification.

Preserved evidence: `out/live-review-20260912/live-game.log`, `Game`, `Options`, and `save-verification.json`.

## Still open / newly reported

- Second FED boss blasters were audible in the first half, but not phase two. Keep the issue open specifically for phase two; do not describe all boss shots as silent. Trace the phase-two firing owner, sample/bank, and attenuation against the shipped executable before choosing a fix.
- Audio rendering is improved. The user reports current music settings were not reflected until selecting the item. Preserved Options has Music ON, music volume 27 and SFX 30; this run began mixer playback at 30 and later used 27. The final Options file alone does not establish the pre-run value or the cause. Initial settings publication and menu entry need investigation.
- Display is incorrect. Preserved Options has ScreenWidth 1920, ScreenHeight 1080, but ResolutionChanged 0; the displayed selector says 640x480. The host log confirms a 1920x1080 source framebuffer and a maximized window. Audit resolution-index synchronization and actual window-mode publication. Do not infer a complete canonical layout from the screenshot alone.
- Startup/legal and objective-intro items have not received explicit live confirmation and remain open.

## Open-list renumbering

Closed items removed. Former #5 → #1; #6 → #2; #7 → #3; #8 → #4; #10 → #5. New Display item is #6. Mod support remains assessed/planned only.

User subsequently confirmed startup/legal and level objectives complete; former TODO 4 and 5 are closed. Display is now #4.
