"""Keep development metadata and the exposed firmware version consistent."""
from pathlib import Path
import argparse
import os
import re

VERSION = r'\d+\.\d+\.\d+(?:-dev)?'
HEADER = re.compile(r'(?m)^(# (?:Release package|ESPHome EasyHome) v)(' + VERSION + r')(?=[. ]|$)')
VALUE = re.compile(r'(?m)^(  firmware_version: ")(' + VERSION + r')(")$')


def versions(root):
    package = (root / 'packages/easyhome.yaml').read_text(encoding='utf-8')
    example = (root / 'examples/aldes-easyhome.yaml').read_text(encoding='utf-8')
    values = [VALUE.search(package), HEADER.search(package), HEADER.search(example)]
    if any(value is None for value in values):
        raise ValueError('Missing version declaration')
    if 'version: ${firmware_version}' not in package or 'return {"${firmware_version}"};' not in package:
        raise ValueError('ESPHome project and sensor must share firmware_version')
    found = [value.group(2) for value in values]
    if len(set(found)) != 1:
        raise ValueError(f'Inconsistent versions: {found}')
    return found[0]


def prepare(root):
    current = versions(root)
    version = current.removesuffix('-dev') + '-dev'
    edits = []
    for relative in ('packages/easyhome.yaml', 'examples/aldes-easyhome.yaml'):
        path = root / relative
        original = path.read_text(encoding='utf-8')
        result = HEADER.sub(lambda m: m[1] + version, original)
        result = VALUE.sub(lambda m: m[1] + version + m[3], result)
        if result != original:
            edits.append((path, result))
    for path, result in edits:
        path.write_bytes(result.encode('utf-8'))
    return [path.relative_to(root).as_posix() for path, _ in edits]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--branch', help='Required for writes; main and tags are rejected')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    if args.check:
        version = versions(root)
        tag = os.environ.get('RELEASE_TAG')
        if tag and (version.endswith('-dev') or tag != 'v' + version):
            raise SystemExit('Release tag must match a production version')
        print('Consistent version: ' + version)
    else:
        if not args.branch or args.branch == 'main' or args.branch.startswith('refs/'):
            raise SystemExit('An explicit non-main branch name is required')
        print('Development version updated: ' + ', '.join(prepare(root)))
