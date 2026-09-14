# Native mod packages (version 1)

Native mod support is deployed in the reconstructed executable. The 14 migrated packages are installed under the game’s `mods` folder. Validation and deployment evidence are recorded in [NATIVE_MOD_SUPPORT.md](NATIVE_MOD_SUPPORT.md).

## Install, disable, remove

Put each package under the game folder as `mods/<package-id>/mod.json`, with its resource files beneath that same folder. Restart the reconstructed executable after changes. Set `"enabled": false` in a manifest to disable a package. Removing its folder also disables it. The original executable continues to use its existing proxy/configuration and original resources.

A character package has this layout:

```text
mods/kitfisto/
  mod.json
  res/MODEL/kitfisto.bmd
  res/animation/kitfisto.cad
  res/combo/kitfisto.cmb
  res/front/NewUI/CharacterSelectImages/kitfisto.png
  res/... referenced textures and other dependencies
```

## Character manifest

This is the manifest generated from the installed Kit Fisto configuration:

```json
{
  "version": 1,
  "type": "character",
  "id": "kitfisto",
  "enabled": true,
  "modelId": 115,
  "name": "Kit Fisto",
  "model": "kitfisto",
  "animationDonor": "mace",
  "forceDonor": "mace",
  "isJedi": true,
  "hidden": false,
  "soundBank": "mace",
  "colors": [
    "0001c03c",
    "1bf07400",
    "0001c03c"
  ],
  "icons": [
    "000000B3",
    "000000b6",
    "000000B3"
  ],
  "bmd": "res/MODEL/kitfisto.bmd",
  "cad": "res/animation/kitfisto.cad",
  "cmb": "res/combo/kitfisto.cmb",
  "portrait": "res/front/NewUI/CharacterSelectImages/Kitfisto.png"
}
```

- `version` is currently `1`; `type` is `character` or `assets`.
- `id` is a stable package identifier. Keep it unchanged across updates: native saves use it to recover character identity. Enabled package IDs must be unique.
- `modelId` is a unique integer from 115 through 254. These are separate from stock model IDs. Numeric IDs may change between runs because saves resolve the stable package ID.
- `name` is the displayed character name. `model` is the resource stem. Internal model-cache names are generated separately, so different packages cannot alias merely by sharing an animation donor or resource stem.
- `animationDonor` supplies stock movement/combo behavior. `forceDonor` selects the Force recipe. Supported names are `obi_wan`, `qui_gon`, `mace`, `adi`, `plo`, `maul_p`, `amidala`, `panaka`, `ki_adi`, and `maul`. Custom CAD/CMB files must retain compatible motion and gameplay-event semantics; the manifest does not remap arbitrary rigs or animation events.
- `isJedi` controls the native package's saber/Force capability. `hidden` removes it from selection while keeping the package available to saved games. `soundBank` names an inherited or packaged SFX bank.
- `bmd`, `cad`, `cmb`, and `portrait` are required existing files, relative to the package directory.
- `colors` and `icons` each contain three eight-digit hexadecimal strings: default, alternate, and current. Colors retain the engine's encoded values. Without `saberIcons`, icons index the existing menu texture table (0–248).
- Optional `saberIcons` contains two existing package-relative image paths, default then alternate. For example: `"saberIcons": ["res/front/NewUI/Lightsaber_Green.png", "res/front/NewUI/Lightsaber_New6.png"]`. These are loaded separately from stock menu slots and follow the selected saber variant. Copy-only legacy migration now includes these files and upgrades otherwise unchanged old manifests with these paths; it does not overwrite unrelated package edits.

## Resource packages

A resource-only package needs this manifest and a `res` subtree:

```json
{"version":1,"type":"assets","id":"my-level-resources","enabled":true}
```

Preserve the existing virtual resource path to replace a resource—for example `res/level/jpx/fed/fed.jpx`. Enabled packages are checked together. Different contents at the same virtual path are rejected; byte-identical shared dependencies are allowed. There is no priority/load-order override system. Omitted resources fall back to the installed game.

The native file, image, audio and level loaders honor these overlays. CAD/CMB, textures, JPX, FBX, collision/camera and other existing resource files can retain their normal paths. FBX supplies rendered level geometry when present, as in the current base pipeline: replacing JPX alone does not replace that separate FBX geometry. This resource contract does not yet register additional levels or provide a Blender exporter/editor.

Paths must remain within their package, including resolved links, and must fit the engine's 255-byte absolute-path limit. The JSON reader rejects malformed data, duplicate keys and excessive size/depth. An invalid enabled package rejects startup of the package set with an error; it is not silently skipped.

## Saves and legacy coexistence

Native saves retain the existing raw `Game` payload and add `Game.mods.json` beside it. The sidecar records stable player package IDs and selected colors/icons, matched to the exact raw save and its modification time. It does not replace the legacy proxy's `jpb_progress.json`. If the original executable rewrites `Game`, unmatched player identity/colors are ignored; the newest native package progression record is retained independently. A matching save that requires an unavailable package cannot be loaded until the package is restored or enabled.

Native mod campaigns checkpoint after successful campaign loading when AutoSave is enabled. Saving uses temporary files and retains the prior matching sidecar record if replacement of the raw save fails. Keep the raw save and its native sidecar together in backups. Completion, best scores, combo masks, award tiers, health/Force upgrades, extra lives and attack/defence bonuses are stored per stable package ID. New mod characters use canonical starting capacities (100 health/Force, growing by 20 per upgrade); gameplay and HUD capacity resolve the package rather than modifying its stock donor. Award eligibility continues to follow the inherited character rules. New Game clears earned native completion, combos and upgrades. Imported legacy unlock baselines remain available.

## Copy existing configured mods

From this repository, use:

```powershell
python tools/migrate_legacy_mods.py "C:\Games\Star Wars Jedi Power Battles" --report "out/migration-report.json"
```

The default destination is the game's `mods` directory; `--destination` can select a staging directory. Migration copies runtime assets and verifies source/destination SHA-256 hashes. It preserves legacy files, refuses conflicting destination content, and can be rerun when existing output matches. Blender source projects are not copied by this command. The original legacy progress sidecar is copied unchanged to `mods/_legacy/jpb_progress.json`; `mods/legacy-progress.json` maps its aggregate level/skill values to stable package IDs. Absent entries receive the proxy’s documented defaults (new Jedi 10/100; other characters 0/0). These are unlock baselines, not invented completion flags, scores or upgrade history.

The installed configuration contains 14 additional characters. Their staged conversion includes BMD/CAD/CMB, portraits, referenced textures and shared animation Huffman tables. The deployed executable discovers these packages automatically. Blender authoring and additional-level registration remain separate work; this release supports existing-format character/animation resources and level-resource overlays.
