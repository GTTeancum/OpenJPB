# Native FMV Natural-End Audit

Generated: 2026-08-31

## Scenario

The linked TheoraPlay backend played movie index `2`
(`HorizontalFlippedQui_converted.ogg`) to natural EOF in the hidden D3D11
window at the normal 60 Hz presentation rate. No skip input was injected.

## Result

- Process exit: `0`
- Requests/resolutions/launches: `1/1/1`
- Source frames decoded: `810`
- Presentation ticks: `1618`
- Decoded audio bytes: `5,185,536`
- Skips/start failures: `0/0`
- Title frames after movie completion: `182`
- Clean shutdown: yes

This proves natural decoder EOF tears down playback and returns ownership to
the existing title loop on the following frame.

Evidence:

The raw playback streams were removed after the decoded-frame, audio-byte,
return-state, and clean-shutdown counters were recorded here.
- `build/Release/jpb_pc_game.log`
