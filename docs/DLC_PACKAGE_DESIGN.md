# Shared DLC package design

Design completed September 14, 2026. This specifies the next package contract;
it is **not implemented in the deployed executable**. Version 1 remains the
supported installation format described in [MODS.md](MODS.md). Closing the
design task does not close runtime integration or Blender export tasks.

## Foundation and decisions

The local implementation establishes the constraints: `mods.cpp` accepts only
version 1 character/assets packages, overlays `res` paths, and saves character
identity by package ID. `mods.h` reserves character slots 115–254. Existing
Blender tools import/export BMD and CAD, and import level geometry and gameplay
data. They do not export complete playable levels. The shared design extends
these systems rather than replacing their working importers or inventing
canonical game behavior.

One package can publish characters, animation sets, levels and reusable assets.
Keep `mods/<package-id>/mod.json` and the existing `res` subtree. Use version 2
for this incompatible manifest change. Separate format version (`version: 2`)
from content release (`packageVersion: "1.0.0"`). Keep authoring sources optional
and outside the runtime resource tree.

```text
mods/author-expansion/
  mod.json
  package.lock.json                   generated resolved dependencies / hashes
  content.sha256.json                 generated runtime file inventory
  res/
    MODEL/hero.bmd
    animation/hero.cad                plus its Huffman tables
    combo/hero.cmb
    front/NewUI/CharacterSelectImages/hero.png
    level/jpx/courtyard/courtyard.fbx  explicit visual geometry
    level/jpx/courtyard/courtyard.jpx  when required by native level processing
    level/W3D/courtyard.j3d
    ... textures, CAM/PWR, audio and other compiled dependencies
  source/                            optional; never read by the game
    hero.blend
    courtyard.blend
    textures/*.png
    animations/*.fbx
    gameplay/*.json
    build.json                       exporter settings and source hashes
```

The directory name normally equals the ID; the manifest ID is authoritative.
Renaming a folder must not rename a character or lose progression. Distribution
may be a ZIP with one such folder; extraction/install remains explicit. No
executable patcher, embedded native DLL, downloaded dependency or install script
is part of this contract.

## Manifest and identity contract

[Two example manifests](examples/dlc-v2/README.md) show a shared animation library
and an expansion consuming it. They are design fixtures, not installable mods.

| Field | Contract |
| --- | --- |
| `version` | Integer 2. Unknown versions reject activation. |
| `id` | Immutable package ID, 1–95 lowercase ASCII letters, digits, `_` or `-`; start with a letter. Prefer `author-project`; `openjpb-` is reserved. Existing v1 IDs retain their identity. |
| `packageVersion` | Exactly three nonnegative decimal integers, `major.minor.patch`, compared numerically; no leading zeros except zero itself. Breaking exported contracts increment major; additions minor; compatible repairs patch. |
| `name` | Display label; can change without affecting references. |
| `enabled` | Boolean, default true; disabled packages cannot satisfy dependencies. |
| `requires` | List of engine capability names. Unknown or unavailable capability rejects activation; capability names have version suffixes such as `levels-v1`. These are planned gates, not claims about today's engine. |
| `dependencies` | Required dependencies only: stable package ID plus inclusive `minVersion` and exclusive `maxVersion`. No wildcards, optional edges, automatic downloads or inferred transitive visibility. Empty list is valid. |
| `exports` | Typed arrays `assets`, `animations`, `characters`, `levels`; omitted arrays mean empty. At least one export or overlay is required. |
| `bindings` | Optional compiled-resource bindings: `owner` export reference, embedded `resource` path/name, and typed `asset` export reference. Owner must be local; asset must be local or from a direct dependency. |
| `overlays` | Explicit mappings of stock virtual paths to package-local files; empty by default. |
| `extensions` | Optional vendor-keyed metadata; no gameplay semantics. Unknown fields outside this object are errors, catching misspelled settings. |

An export has an immutable local `id` using the same identifier grammar. Its
stable reference is `<package-id>:<kind>/<local-id>`, where kind is `asset`,
`animation`, `character` or `level`. A reference must resolve to the declared
kind in this package or a direct dependency. Display names, folder paths,
Blender object names and allocated engine integers are never identity.

Built-in behavior references use a separate `base:<kind>/<name>` namespace,
for example `base:character/obi_wan`. Resolve only an engine-maintained registry
derived from existing source tables. There is no fallback from a missing custom
export to a donor or a similarly named file.

Blender objects representing persistent placements, cameras, triggers, scripts,
waypoints and level starts receive UUIDs stored in custom properties. Renaming
preserves them; duplication generates a new UUID. Import retains original
archive indices alongside UUIDs. Compilation allocates native indices and
rewrites links in one pass; references never depend on Outliner order.

## Resolution, paths and compatibility

Only one enabled release of a package ID may exist. Resolve all dependencies,
version intervals and capabilities before registering anything. Dependencies
load first, ties sorted by ID. Reject cycles with the complete cycle, duplicate
IDs, missing/disabled dependencies, incompatible releases and dangling exports.
Ordering confers no override priority. Commit the resolved registry atomically;
a failed activation leaves the prior registry intact. Restart applies changes.

`package.lock.json` has its own `version: 1` and a `packages` map from dependency
ID to exact release and manifest/content SHA-256; it covers the full transitive
closure. The add-on/build tool creates it. Runtime verifies it; mismatched
content requires an explicit rebuild, even if the release string matches.
`content.sha256.json` (`version: 1`, `files` map of relative path to SHA-256)
covers every runtime payload beneath `res`, excluding itself and optional
sources. The lock has no dependency on the owning package's inventory, avoiding
recursive hashes. Rebuilding with unchanged inputs produces unchanged manifests,
compiled payloads and inventories; timestamps stay out of reproducible output.

Payload paths are package-relative forward-slash paths under `res/`. Reject
absolute paths, traversal, escaped links and case-insensitive collisions.
Preserve existing resource subdirectories and enforce the current native
255-byte resolved-path limit until the loader is widened. References to another
package use export references, never `../` paths. A scoped resolver first binds
an export's declared files and compiled internal texture/table dependencies to
its package; shared files bind to dependency asset exports. Build inventory must
enumerate these implicit BMD/CAD dependencies as well as manifest paths.
For an embedded resource name, an explicit owner/resource binding takes
precedence over the owning package's matching path; then allow a known stock
resource where the content contract permits inheritance. Duplicate bindings
are errors. Unresolved custom names reject the build. Matching uses normalized
case and separators, consistent with the native resource lookup. Animation
exports own their CAD/CMB/Huffman dependencies, so consumers do not copy them
into each character package.

Version-2 `res` files are private unless explicitly overlaid; installing a new
hero cannot accidentally replace another hero's texture with the same filename.
An overlay record is `{ "target": "res/...", "file": "res/..." }`. Distinct
bytes targeting the same case-normalized virtual path reject the package set;
identical bytes may coalesce. Stock resources supply omitted stock paths. This
preserves v1 conflict behavior without introducing load-order overrides.

Version 1 remains readable unchanged. Adapt a v1 character internally to
`<old-id>:character/main`, keep its requested numeric slot, existing overlays,
donors, files and colors. Adapt v1 assets as legacy overlays. Do not assign v1
an invented semantic release or let it satisfy v2 versioned dependencies:
explicitly publish a v2 library for shared references. v2 character slots are
allocated deterministically from remaining 115–254 slots after v1 reservations.
Reject capacity exhaustion; never truncate the roster. This design does not
promise more than the current 140 added-character slots.

Upgrading an existing character package retains its package ID and export
`main`. The sidecar reader maps old package-only keys to that reference. A new
sidecar version stores full export references, selected variant ID, relevant
package versions and level UUID progression; numeric IDs are temporary. Keep
raw-save/sidecar matching and legacy `jpb_progress.json` coexistence. Missing
required content must explain what is unavailable and prevent that save loading,
not silently substitute a donor. Reinstallation restores access. Removal does
not delete saved progression. Sidecar migration writes a new file atomically
and retains the old file; it never rewrites legacy mod assets.

## Typed exports

**Assets:** `id`, `format`, `file`. Initial formats are `png`, `tga`, `wav`,
`fbx`, `bmd` and `binary` (explicit native dependencies). Formats are validated
against actual contents. Asset publication does not imply a stock overlay.

**Animations:** `id`, `rig`, `cad`, `cmb`, `huffman` (exactly three paths in
tab/val/opt order), `motionProfile`. `rig` and `motionProfile` use built-in
registries initially, e.g. `base:rig/obi_wan` and
`base:motion-profile/obi_wan`. Export compilation checks node IDs/hierarchy,
weapon attachments and required motion slots/events against that profile.
Sharing a CAD filename or donor label alone does not establish compatibility.
Custom rig/profile registration is a later capability, rejected until supported.

Use Blender Actions/NLA for editing and bake the intended Action per clip.
Preserve native Motion/CMB metadata, authored frame counts, cut-in/out, damage,
sound callbacks and per-frame events. Retain 30 authored frames/second and
existing CAD precision. Do not lengthen gameplay windows to accommodate visual
animation changes without an explicit event/timing edit. BMD remains rigid-joint
geometry; unsupported multi-weight skinning, bone translation/scale or excessive
vertices fail export rather than silently degrading.

**Characters:** `id`, `name`, `bmd`, `portrait`, `animation` (animation export
reference), `behavior` and `forceBehavior` (stock character references), `soundBank`
(existing stock sound-bank reference initially), `isJedi`, `hidden`,
`saberVariants`. Each variant has stable `id`, eight-digit native `color` and
package-local `icon`. Jedi require exactly two variants for the current menu;
non-Jedi use an empty array. The first variant is the default; current selection
belongs in save/user state, not mutable manifest data. Behavior remains explicit
even when an animation set is shared. Importing a mesh never guesses its donor.

**Levels:** `id`, `name`, `modes` (nonempty subset of `campaign`, `versus`),
`players` (`min`/`max`, 1 or 2), `preview`, `visual` (`format` and `file`),
`world` (J3D), `camera` (CAM), `pickups` (`onePlayer`/`twoPlayer` PWR), `starts`
(player start UUIDs), `next` (level reference or null), `completion` (script/event
UUID), `saveRevision` (positive integer). Required PWR/start records must cover
every declared player count. Visual format is `fbx` or `jpx`, selected explicitly;
FBX and JPX must not silently compete. Any supporting archive needed by native
processing remains declared in the build inventory.

The level compiler validates collision/spatial data, actor references, camera
regions, trigger links and script graph edges against decoded local runtime
contracts. Compile known native opcodes; retain raw unknown imported data only
for an unchanged compatible round trip. Unknown edited commands or invalid
relocations fail the build. No arbitrary Python executes in the game.

New levels require a registry mapping stable references to runtime level data,
menu registration and completion transitions, replacing fixed-index assumptions.
Completion is explicit, not inferred from an empty enemy list. `next: null`
returns to the appropriate menu after results. Versus ignores campaign `next`;
versus-only levels require null. Separate progression keys by character and
stable level reference. A changed `saveRevision` invalidates incompatible
in-level checkpoints with an explanation while preserving completion records.
Implementing that registry/compiler remains a separate task; v1 overlays cannot
currently add a campaign chapter or a second VS map.

## Blender workflow and implementation boundary

Extend `tools/blender` and its existing installable add-on. Users edit meshes,
UVs, materials, armatures, Actions, cameras and collections with Blender's own
tools. Keep `.blend` as the authoring master, PNG/TGA textures, WAV source audio,
FBX interchange and versioned JSON gameplay metadata. Standard formats carry
geometry/animation; they do not replace the game-specific event semantics.
Do not introduce a separate scene editor or require a bespoke intermediate
mesh format.

Add Package Properties, typed export assignment, dependency selection, validation
results that select offending objects, and **Build Package** to the existing JPB
sidebar. Reuse the imported Script node trees and camera/trigger inspectors.
Store authored gameplay metadata in a `version: 1` JSON document with UUID-keyed
records and explicit links, preserving original records/source hashes on import.
Its detailed opcode schemas follow local decoder/compiler work, not guessed
editor conventions.

Build to a separate staging folder, validate all references/compiled data and
write inventories/lock files only on success. Then copy the complete package
to `mods`; retain legacy files and user sources. The same exporter can run under
Blender `--background` for automation; no desktop input is required. Native BMD,
CAD and level payloads remain build products, not the only editable originals.

Implementation order: (1) v2 parser, resolver, scoped resources, v1 adapter and
save migration; (2) shared animation binding and character package builder;
(3) level compiler/registry/progression; (4) integrated Blender publishing.
Each stage advertises only implemented capabilities. Until then current v1
packages remain the installation route and level export stays unavailable.

Acceptance before claiming implementation complete: validate missing/conflicting
dependencies, cycles, path escapes, version mismatches, stable IDs after folder
renames/updates, capacity exhaustion and v1 save recovery. Publish a shared
animation library used by two characters, a modified stock level and a new 2P VS
map. Drive actual title/menu selections through process-local virtual input;
inspect native captures for custom models, textures, animation/events, backgrounds,
media and transitions; capture/check audio where used. Verify saves, reloads,
completion, camera/trigger behavior and both players. A rendered frame alone is
not acceptance evidence. No runtime or visual validation is claimed by this
design-only change.
