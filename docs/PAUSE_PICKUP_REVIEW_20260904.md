# Pause and Pickup Review - 2026-09-04

## Authority

Direct disassembly of the locally installed `game.exe`, using the matching PDB
function identities. SHA-256:
`88932504124D58315030C4CE8D03B5B6AC02AB57E90BB84A8BC7D1F0CE1AE05F`.
Addresses below are RVAs. No online or visual-approximation implementation evidence.

## Ultimate Saber

- `menu_ultimate` (`0xBF290`, 13 bytes) returns `(secretBits >> 8) & 1`.
- `gamepauseMenuMdef` (`0x4C7CC0`) prefixes its Ultimate Saber record with
  conditional command `[18, 6]`. The option is unlock-gated, not always present.
- `mmDrawsub` command 18 (`0xD1B06`) dispatches callback table `0x4CAA50` on
  the following command. A false result decrements `mmTotal`, increments
  `mmSubSet`, and skips that following command for both drawing and selection.
- The reconstruction had omitted command 18. Restored all nine non-null
  condition callbacks, including the Ultimate Saber predicate at slot 6.
- Locked saves now have five selectable Pause entries; unlocked saves have six.
  Quit remains reachable in both. This does not remove the unlock or the option.

## Controls Layout

Compared the complete `runControlsMenu` (`0xD81F0`, 4991 bytes), including all
resolution branches, keyboard, controller, and second-player drawing.

- Restored the Controls heading's `3.0` scale.
- Restored Walk/Run sliders: scale `0.225`, maximum `8`, row separation `45`,
  second-player X offset `900`, and all seven resolution-specific origins.
- Corrected resolution-specific legend centers, including mode 3's force
  center `5` and default mode's primary/force centers `-310`/`25`.
- Every row uses the first action's texture dimensions for its icon rectangle.
- Text offsets are added after pivot conversion: primary labels `65 * scale`,
  force labels `150 * scale`, keyboard force labels `65 * scale`.
- Restored modifier/second-button placement and mode 5's per-row Y adjustments.

New unit coverage checks locked/unlocked Pause selection and 56 Controls
combinations: 540p/1080p, seven resolution modes, keyboard/controller, and
Classic/Modern settings. Existing end-to-end Quit fixtures were corrected to
wrap upward from Continue to Quit instead of counting the erroneously visible
Ultimate Saber entry. The locked Pause render assertion expects eight text
draws including the two HUD text draws, not nine.

Nine Release gates passed: menu, runtime-title, powerup, sound, front-end
Pause/Controls, Pause presentation, Pause/Quit/title, and post-Quit Versus
entry for both player counts. The menu unit gate also passed after restoring
the command-18 write of `mmsizes[0] = 2`.

Important visual-coverage limit: the 1080p headless front-end capture reaches
Controls (mode 35, stack 5), but retains game mode 4. The menu owner therefore
draws its title background, unlike the user's live gameplay capture. These
route gates prove navigation and drawing activity, not full Pause presentation
parity. This gap remains in TODO item 2. No test save was written.

The physical-XInput-mapping front-end regression also passed after correcting
the process-local headless controller-name hook. Separate native 1920x1080
keyboard and controller framebuffer captures show separated labels and button
combinations. They are layout evidence only, subject to the background-state
limitation above; they are not a live presentation sign-off.

## Pickup Audio Assessment

- `pwrup_Update` (`0xEA2E0`) calls `sound_Play` with resident bank `0`, the
  player's position, `xsecret`, and flags `0`; examples at `0xEA93C` and
  `0xEA987`. Saber-upgrade calls use `xsaberup`; one also requests `xsecret`.
- `sound_Play` (`0x12AFC0`) and `sound_playSfx` (`0x12B3E0`) preserve those
  arguments. No pickup-specific quiet prefix or volume multiplier is supplied.
- `get_sound_volume` (`0x12A7B0`) uses camera-relative horizontal distance and
  panning. The reconstructed distance calculation matches the executable;
  distance attenuation reaches `171/255` at its far limit.
- The host passes panning, distance, and channel volume directly to SDL_mixer.
- The user's 56-byte Options file at the time of this report contains SFX
  volume `30`; the menu's maximum is `75`. No settings were changed.
- Both resident WAVs are mono, 44.1 kHz, 24-bit PCM. Correctly decoded signed
  24-bit measurements: `xsecret` duration 0.570 s, peak -3.62 dBFS, RMS -15.92
  dBFS; `xsaberup` duration 0.701 s, peak -0.04 dBFS, RMS -10.35 dBFS.

These findings explain possible quietness, but are not a listening sign-off or
proof that the reported in-game mix is correct. No arbitrary gain boost was
applied. The audibility report remains open.

### Gain Recheck - 2026-09-05

Repeated the direct executable trace rather than adding a speculative gain:

- Pickup calls at `0xEA92D..0xEA93C` and `0xEA978..0xEA987` supply the
  player's position, bank zero, `xsecret`, and flags zero.
- `sound_playSfx` at `0x12B490..0x12B53F` takes the positional branch for
  `xsecret` and `xsaberup`. Neither receives the `-` quiet modifier or the
  `v` voice modifier at `0x12B53F..0x12B594`.
- `0x12B744` loads `1.0f` from `0x45F938`. Only the 2D branch replaces it
  with `0.92f` from `0x39C994` at `0x12B7BB`. `0x12B7C3..0x12B7DB`
  multiplies the saved SFX byte by that factor and passes the truncated
  result to `Mix_Volume`. There is no additional pickup boost.
- PDB `optionstruct.SFXVolume` is byte offset 11. The EXE's
  `defaultOptionStruct` at `0x4BAB98` contains 30 there, matching the user's
  current saved value. That value is not evidence of a bad save.
- The host loads the installed `SDL2_mixer.dll`, loads the original WAV via
  `Mix_LoadWAV_RW`, and directly forwards panning, distance, and channel
  volume. It does not normalize the sample or apply a second volume factor.
- Direct DLL checks of `Mix_Volume` (`0x2D3C0`) and `Mix_VolumeChunk`
  (`0x2D450`) confirm that querying with `-1` returns the stored value and
  that their full-scale setting is 128. These two addresses are DLL RVAs.

Added 48 regression cases covering both cues, mono/stereo, SFX values
0/30/75, and horizontal distances 512/1536/2048/2560. They enforce distance
values 0/57/114/171, centered stereo panning 127/127 (mono 255/255), unchanged
SFX volume, bank zero, and flags zero. The deliberately different height
also guards the executable's horizontal-only distance calculation.

Two native host probes load the real resident WAVs through the installed
mixer, call the normal positional `sound_Play` path, and query its actual
channel/sample volume. Both report channel volume 30 and sample volume 128;
there is no hidden sample-volume reduction. These probes use SDL's dummy
device in CTest and are gain/routing evidence, not a human listening test.
All six focused Release gates pass: sound, powerups, host audio, both pickup
gain probes, and the movie/SFX gate.

Remeasuring the unmodified 24-bit assets confirms `xsecret` is 5.57 dB lower
in RMS than `xsaberup`, before the canonical spatial mix. Asset SHA-256:

- `xsecret.wav`: `27CC70700B9FA9BBC7AEC52D854F82334A0F16AF4938124A3F4A40BE0FB1ECD3`
- `xsaberup.wav`: `055A3597642F1D386144E72CA963B9D10C2C5B1F9D1BEA93C576B08FB2604107`

No reconstruction mismatch was found in pickup gain. No production gain,
assets, saved settings, or installed executable were changed. Item 5 remains
open for LIVE audibility review, not an unimplemented gain correction.

## Other Playtest Outcomes

- User signed off the FED door sequence and conveyor animation. Removed their
  completed TODO section; reused slot 5 for pickup audio to retain boss slot 6.
- FMV desynchronization remains a confirmed open failure. No FMV repair is
  claimed by this change.
- FED boss introduction lock added to item 6 alongside health/damage/death.
  The installed game log records dolly 136, `action_lock=1` at frame 27072,
  idle at 27075, then shutdown about 15 seconds later without a logged release.
  This is evidence to reproduce, not proof of the lock's root cause.
