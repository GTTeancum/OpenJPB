# Core Darth Maul Lifecycle Audit

Generated: 2026-08-31

## Canonical Evidence

- Direct `core.j3d` inspection resolves placement `11` to actor `4`, AI `33`,
  owner `2`, 250 HP, and model `43` through the executable-recovered
  `loader_loadEnemies` mapping.
- AI program `33` has 141 stored nodes and no `0x60f` extension-spawn opcode.
- Placement `11` has `nLink=0`. Its `enemyExt[0]=64` is authored AI data, not
  a delete-link. The previous diagnostic label `link0` and the resulting claim
  that death must launch an AI-33 extension transition were incorrect.
- The later placements `31..34` are AI `47`, owner `3`, range-activated Maul
  placements. They are independent encounter stages rather than delete-links
  from placement `11`.

## Runtime Proof

An explicit process-local lifecycle run forced placement `11`, seeded only its
energy to `1`, then used the reconstructed attack, hit, damage, death animation,
enemy deletion, and placement ownership paths unchanged.

- Process exit: `0`
- Attack contacts: `1`
- Damage events processed: `1`
- Energy: `1 -> 0`
- Placement `11` after 300 frames: `status=2`, handle cleared
- Active enemy actors after cleanup: `4` (peak `5`)

The first bounded run exposed a harness bug: its periodic combat-positioning
pulse treated an already-dead target as a failure. The harness now repositions
only a live owner-2 combat target, so normal death removal is not intercepted.

The retained counters above are the compact evidence record; the raw diagnostic
stream was removed after verification.
