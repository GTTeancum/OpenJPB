"""Generate out-of-tree nxdk compilation wrappers; shared sources stay shared."""
from pathlib import Path
import os
import argparse
import re

parser = argparse.ArgumentParser()
parser.add_argument('--ufbx', type=Path, default=Path.home() / '.codex/deps/ufbx-0.6.1')
args = parser.parse_args()
if not (args.ufbx / 'ufbx.h').is_file() or not (args.ufbx / 'ufbx.c').is_file():
    parser.error('--ufbx must name the local matched ufbx 0.6.1 source tree')

root = Path(__file__).resolve().parents[2]
target = root / 'xbox'
# Windows clang emits backslash-separated include names. Normalize separators
# while preserving Make's escaped spaces and line continuations.
for dep in target.rglob('*.d'):
    contents = dep.read_text()
    dep.write_text(re.sub(r'\\(?=[^\s])', '/', contents))
generated = target / 'build/shared'
generated.mkdir(parents=True, exist_ok=True)
sources = []
for source in sorted((root / 'src/reconstructed').rglob('*')):
    if source.suffix not in ('.c', '.cpp'):
        continue
    relative = source.relative_to(root / 'src/reconstructed')
    if relative.as_posix().startswith(('original/d3d/', 'original/rendering/font/', 'original/platform/steam/')) or relative.as_posix() in ('original/wHook.cpp', 'original/rendering/Texture2D.cpp', 'portable/mods.cpp'):
        continue
    wrapper = generated / relative
    wrapper.parent.mkdir(parents=True, exist_ok=True)
    prefix = ''
    if source.suffix == '.cpp':
        prefix = '#include <algorithm>\n#include <array>\n#include <cmath>\n#include <codecvt>\n#include <cstdio>\n#include <cstring>\n#include <cwchar>\n#include <limits>\n#include <locale>\n#include <map>\n#include <memory>\n#include <set>\n#include <stdexcept>\n#include <string>\n#include <vector>\n#undef _WIN32\n'
    contents = prefix + '#include "' + os.path.relpath(source, wrapper.parent).replace('\\', '/') + '"\n'
    if not wrapper.exists() or wrapper.read_text() != contents:
        wrapper.write_text(contents)
    sources.append(wrapper.relative_to(target).as_posix())
wrapper = generated / 'ufbx_native.c'
wrapper.write_text('#define UFBX_STANDARD_C 1\n#include "' + (args.ufbx.resolve() / 'ufbx.c').as_posix() + '"\n')
sources.append(wrapper.relative_to(target).as_posix())
(target / 'build/sources.mk').write_text('SHARED_SRCS := ' + ' '.join(sources) + '\nUFBX_INCLUDE := ' + args.ufbx.resolve().as_posix() + '\n')
print(f'Configured {len(sources)} shared translation units')
