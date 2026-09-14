# Animation state crossfades

Implemented after mod-support checkpoint dbcce12 was pushed to OpenJPB/master.

The existing CAD decoder interpolates frames within a sequence. The new portable blend layer crossfades joint angles when the sequence or motion changes, using the last displayed pose as the source. An interrupted blend starts from its current displayed pose, avoiding a second snap. The default duration is 200 ms with a smoothstep curve and shortest-path 12-bit angle interpolation.

Players, the second player and active enemies each own independent blend state. New actors initialize directly at their first pose; pauses do not advance the blend. Authored animation clocks, root translation, event bytes and callbacks remain current. Rendered joint centers and saber attachments follow the blended pose, so collision geometry follows that displayed pose during the short transition rather than a separate invisible skeleton.

Use `--animation-blend-ms 0` for the unblended path, or a value from 0 to 250 to tune general transitions. Motions with damage or attack flags remain capped at 100 ms; smaller overrides and disabling still apply to them. This is an explicitly requested enhancement, not a claim of stock EXE behavior. The recovered angle convention and frame layout are used unchanged.

## Verification

- Model-pose tests cover wraparound, endpoints, interrupted transitions, pauses, independent actors, disabling, untouched source data and authored root/events.
- Native A/B runs cover stock Obi-Wan and a pair of mods sharing Maul's animations. Both enabled runs record seven P1 transitions and exit cleanly; disabled runs record zero. Evidence: out/animation-blending/results.json and the associated logs.
- Matched native captures at frames 22, 25 and 29 show the transition before, during and after the crossfade. The middle pose eases into the run; the completed pose agrees with the target. Captures were inspected in transition-comparison.png.
- AI-enabled native run records P1/P2/enemy transitions 1/3/48 and clean shutdown. The first attempt mixed attack assertions with an authored AI-controlled introduction, so its attack validator failed; the corrected AI-only run removes those inapplicable player-attack expectations. Explicit player attacks are covered by the separate A/B runs.
- Final regression/Force verification and deployment status are recorded below when completed.

## Deployed for live review

- All 97 unit regressions pass after a full Release build. Four hybrid Force characters also retain repeated activation, expected Force use and movement recovery.
- Deployed executable SHA-256: 7e521e7c11c548deb8f4f4a1edbfde2711c86c5395e651917d74c9300bddab4d. The prior mod-support executable is backed up in out/animation-blending/before-animation-blending.exe; original-executable assets/configuration/progress remain unchanged.
- Mod-support checkpoint dbcce12 is committed and pushed. The subsequent blending enhancement remains a separate working-tree change for live review.

## September 13: slightly longer general transitions

Raised the general default from 100 to 150 ms after live feedback. Incoming motions with `Damage` or `attackFlags` retain the previous 100 ms cap. Movement and return-to-idle ease longer while attack startup gets no additional blend time. Animation clocks, root translation and events remain unchanged; displayed joint/collision positions still follow the blend. Regression and native comparison evidence: `out/animation-blending-150`. The game executable is now named `OpenJPB.exe`.

September 13: increased movement/recovery to 200 ms at user request; the 100 ms incoming-attack cap remains.
