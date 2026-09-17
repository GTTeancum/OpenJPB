# Version-2 design examples

These JSON files illustrate [the shared package contract](../../DLC_PACKAGE_DESIGN.md).
They contain invented example content and no game payloads. Do not install them:
the current game rejects version 2, and the referenced assets are not supplied.

- `shared-animation.mod.json` would become
  `mods/example-jedi-motion/mod.json`, publishing an animation export.
- `expansion.mod.json` would become `mods/example-courtyard/mod.json`, consuming
  that export and publishing a character, a reusable texture and a new VS level.

Paths are examples beneath each package, not assertions about existing stock
filenames. The level UUIDs must resolve to compiled gameplay records when built.
Native saber color values are opaque engine-encoded values, not HTML colors.
The renderer and exporter must preserve those encodings.

The design review checked JSON parsing, unique identities, dependency version
compatibility, typed animation resolution, UUID syntax, required player starts,
saber variant counts and contained path syntax. Payload decoding, Blender
export, capability support and playability remain implementation acceptance work.
