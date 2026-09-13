"""Copy legacy character assets into native mods/<id>/res packages.

No original files are modified. Existing destination content is retained;
conflicting files abort migration instead of overwriting edited packages.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def migrate(root, destination):
    config = json.loads((root / 'jpb_config.json').read_text(encoding='utf-8'))
    root = root.resolve()
    destination = destination.resolve()
    roster = {e["entity"]: e for e in config.get("roster", [])}
    plans = []
    manifests = []
    progress_source = root / 'SAVEDATA0/jpb_progress.json'
    progress = {}
    if progress_source.exists():
        document = json.loads(progress_source.read_text(encoding='utf-8'))
        if document.get('version') != 1 or not isinstance(document.get('entities'), dict):
            raise ValueError('Invalid legacy progress document')
        progress = document['entities']
        plans.append((progress_source, destination / '_legacy/jpb_progress.json'))
    imported = dict(version=1, packages={})
    for entity in config.get('newEntities', []):
        model = entity['model']
        if not re.fullmatch(r'[a-z0-9_-]+', model):
            raise ValueError(f'Invalid model name: {model}')
        package = destination / model
        # Proxy init_progress_sidecar seeds new Jedi to 10/100; persisted
        # entries override those aggregates. They are not gameplay scores.
        summary = progress.get(str(entity['entity']), dict(level=10 if entity['isJedi'] else 0, score=100 if entity['isJedi'] else 0))
        level, skill = summary.get('level'), summary.get('score')
        if type(level) is not int or type(skill) is not int or not 0 <= level <= 10 or not 0 <= skill <= 100:
            raise ValueError(f'Invalid legacy progress for {model}')
        imported['packages'][model] = dict(highestLevel=level, skillPercent=skill)
        base = entity['baseCharacter'].split('-', 1)
        anim, force = base[0], base[-1]
        manifest = dict(version=1, type='character', id=model, enabled=True,
                        modelId=entity['entity'], name=entity['name'], model=model,
                        animationDonor=anim, forceDonor=force,
                        isJedi=entity['isJedi'], hidden=entity.get('hidden', False),
                        soundBank=anim,
                        colors=[entity[k] for k in ('defColor', 'altColor', 'curColor')],
                        icons=[entity[k] for k in ('defIcon', 'altIcon', 'iconDisp')])

        def add(source, relative):
            if relative.is_absolute() or '..' in relative.parts or ':' in str(relative):
                raise ValueError(f'Invalid package path: {relative}')
            source.resolve().relative_to(root)
            (package / relative).resolve().relative_to(destination)
            if not source.is_file():
                raise FileNotFoundError(source)
            plans.append((source, package / relative))
            return relative.as_posix()

        manifest['bmd'] = add(root / f'res/MODEL/{model}.bmd', Path(f'res/MODEL/{model}.bmd'))
        for ext, folder in [('cad', 'animation'), ('cmb', 'combo')]:
            choices = [root / f'res/{folder}/{model}.{ext}',
                       root / f'res/animation/{model}.{ext}',
                       root / f'res/{folder}/{anim}.{ext}']
            source = next((p for p in choices if p.is_file()), choices[-1])
            manifest[ext] = add(source, Path(f'res/{folder}/{model}.{ext}'))
        for name in ('huffman.tab', 'huffman.val', 'huffman.opt'):
            relative = Path('res/animation') / name
            add(root / relative, relative)
        portrait = entity.get('portrait', '')
        relative = Path('res/front') / portrait
        manifest['portrait'] = add(root / relative, relative)
        bmd = (root / f'res/MODEL/{model}.bmd').read_bytes()
        textures = {m.decode('ascii') for m in re.findall(rb'([A-Za-z0-9_]+)\.bmp', bmd, re.I)}
        for texture in sorted(textures):
            relative = Path(f'res/MODEL/tga/{texture}.tga')
            add(root / relative, relative)
        sound_override = roster.get(entity['entity'], {}).get('soundOverride', entity.get('soundOverride', 'inherit'))
        if sound_override not in ('inherit', 'new'):
            raise ValueError(f'Unsupported sound override for {model}: {sound_override}')
        if sound_override == 'new':
            manifest['soundBank'] = model
            bank = root / f'res/sound/sfx/final/{model}'
            if not bank.is_dir():
                raise FileNotFoundError(bank)
            for source in bank.rglob('*'):
                if source.is_file(): add(source, source.relative_to(root))
        manifests.append((package / 'mod.json', manifest))

    progress_target = destination / 'legacy-progress.json'
    if progress_target.exists() and json.loads(progress_target.read_text(encoding='utf-8')) != imported:
        raise ValueError(f'Preserving existing imported progress; conflict: {progress_target}')

    # Validate the complete plan before copying anything.
    unique = {}
    for source, target in plans:
        expected = digest(source)
        if target in unique and unique[target][1] != expected:
            raise ValueError(f'Conflicting migration sources: {target}')
        if target.exists() and digest(target) != expected:
            raise ValueError(f'Preserving modified destination; conflict: {target}')
        unique[target] = (source, expected)
    for target, manifest in manifests:
        if target.exists() and json.loads(target.read_text(encoding='utf-8')) != manifest:
            raise ValueError(f'Preserving existing manifest; conflict: {target}')
    result = []
    for target, (source, expected) in unique.items():
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists(): shutil.copy2(source, target)
        assert digest(source) == digest(target) == expected
        result.append(dict(source=str(source), destination=str(target), sha256=expected))
    for target, manifest in manifests:
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists(): target.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    if not progress_target.exists():
        progress_target.write_text(json.dumps(imported, indent=2) + '\n', encoding='utf-8')
    return dict(packages=len(manifests), files=len(result), copies=result, importedProgress=str(progress_target))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_root', type=Path)
    parser.add_argument('--destination', type=Path)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = args.game_root.resolve()
    report = migrate(root, args.destination.resolve() if args.destination else root / 'mods')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f"Copied {report['files']} files into {report['packages']} native packages; originals retained.")
