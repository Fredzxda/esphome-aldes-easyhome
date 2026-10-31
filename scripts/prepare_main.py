"""Remove explicitly marked development instrumentation when promoting to main.

Only packages/, components/ and examples/ are touched. No arbitrary paths or git
commands are accepted. --check is read-only; --strip is idempotent.
"""
from pathlib import Path
import argparse,re

MARKER=re.compile(r'^\s*(?:#|//)\s*DEV_ONLY_(BEGIN|END|FILE)\s*$')
SUFFIXES={'.yaml','.yml','.h','.cpp','.py'}

def cleaned(text):
    lines=text.splitlines(keepends=True)
    if lines and MARKER.fullmatch(lines[0].strip()) and 'DEV_ONLY_FILE' in lines[0]:
        return None
    out=[];inside=False
    for line in lines:
        m=MARKER.fullmatch(line.strip())
        if m:
            if m[1]=='BEGIN' and not inside:inside=True
            elif m[1]=='END' and inside:inside=False
            else:raise ValueError('Unbalanced or nested DEV_ONLY marker')
        elif not inside:out.append(line)
    if inside:raise ValueError('Unterminated DEV_ONLY block')
    return ''.join(out)

def promote(root,strip=False):
    edits=[]
    for folder in ('packages','components','examples'):
        for path in sorted((root/folder).rglob('*')):
            if not path.is_file() or path.suffix not in SUFFIXES:continue
            original=path.read_text(encoding='utf-8')
            result=cleaned(original)
            if path==root/'packages/easyhome.yaml' and result is not None:
                result=re.sub(r'(url: https://github.com/Fredzxda/esphome-aldes-easyhome\s+ref:) [^\n]+',r'\1 main',result)
            if result is not None and path in (root/'packages/easyhome.yaml', root/'examples/aldes-easyhome.yaml'):
                result=re.sub(r'(\d+\.\d+\.\d+)-dev\b',r'\1',result)
            if result!=original:edits.append((path,result))
    # Parse all markers before writing any file, so malformed input is atomic.
    if strip:
        for path,result in edits:
            if result is None:path.unlink()
            else:path.write_bytes(result.encode('utf-8'))
    return [p.relative_to(root).as_posix() for p,_ in edits]

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    modes=parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--check',action='store_true')
    modes.add_argument('--strip',action='store_true')
    args=parser.parse_args()
    changes=promote(Path(__file__).resolve().parents[1],strip=args.strip)
    print(('Removed/promoted: ' if args.strip else 'Development content: ')+', '.join(changes))
    if args.check and changes:raise SystemExit(1)
