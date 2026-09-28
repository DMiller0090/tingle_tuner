"""Set up (and optionally run) decomp-permuter on one function.

    python tools/permuter/perm.py GZLJ01 Effect8 --comment           # near-miss comment in src/
    python tools/permuter/perm.py GZLJ01 Effect8 file.c              # a file with the function
    python tools/permuter/perm.py GZLJ01 Effect8 --comment --run -j 8 [-- permuter args]
    python tools/permuter/perm.py GZLJ01 Effect8 --run -j 8          # rerun an existing dir

Writes build/<ver>/permuter/<func>/ with what the permuter wants: base.c (the source run
through cpp, other functions cut down to prototypes, INCLUDE_ASM/top-level asm() dropped,
`F_(...) asm("F")` renames moved into renames.sed, static data turned into extern
declarations so relocations name the same symbols as the original), target.o (the original
function assembled from build/<ver>/asm/funcs/<func>.s), compile.sh (agbcc + as, from
tools/permuter/compile.sh) and settings.toml. The permuter runs in WSL (python3 with
pycparser and toml; binutils-arm-none-eabi on PATH or unpacked in ~/arm-binutils).
Clone it with `git clone https://github.com/simonlindholm/decomp-permuter ~/decomp-permuter`
in WSL, or set PERMUTER to the clone's path (as seen from WSL). Results land in
build/<ver>/permuter/<func>/output-<score>-<n>/ (source.c + diff.txt).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from build import unix_path, WSL_DISTRO  # noqa: E402
from common import BUILD, ROOT, versions  # noqa: E402
from scratch import uncomment  # noqa: E402

HERE = Path(__file__).resolve().parent
PERMUTER = os.environ.get('PERMUTER', '~/decomp-permuter')


def wsl(script, capture=True):
    cmd = ['bash', '-c', script] if os.name != 'nt' else ['wsl', '-d', WSL_DISTRO, '--', 'bash', '-c', script]
    if not capture:
        return subprocess.run(cmd).returncode
    r = subprocess.run(cmd, capture_output=True)
    out = r.stdout.decode('utf-8', 'replace')
    if r.returncode:
        sys.exit(out + r.stderr.decode('utf-8', 'replace') + f'\nfailed: {script}')
    return out


def match_paren(text, i):
    """Index of the bracket closing the one at text[i]."""
    pairs = {'(': ')', '{': '}', '[': ']'}
    stack = []
    j = i
    while j < len(text):
        c = text[j]
        if c in '"\'':
            j = skip_string(text, j)
            continue
        if c in pairs:
            stack.append(pairs[c])
        elif c in ')}]':
            stack.pop()
            if not stack:
                return j
        j += 1
    raise ValueError('unbalanced')


def skip_string(text, i):
    q = text[i]
    j = i + 1
    while text[j] != q:
        j += 2 if text[j] == '\\' else 1
    return j + 1


def top_level_chunks(text):
    """Split preprocessed C into top-level declarations/definitions."""
    chunks = []
    start = i = 0
    while i < len(text):
        c = text[i]
        if c in '"\'':
            i = skip_string(text, i)
            continue
        if c in '([':
            i = match_paren(text, i) + 1
            continue
        if c == '{':
            end = match_paren(text, i)
            if text[start:i].rstrip().endswith(')'):  # a function body
                chunks.append(text[start:end + 1])
                start = i = end + 1
                continue
            i = end + 1
            continue
        if c == ';':
            chunks.append(text[start:i + 1])
            start = i + 1
        i += 1
    if text[start:].strip():
        chunks.append(text[start:])
    return [c.strip() for c in chunks if c.strip()]


def name_before_paren(chunk, close):
    """Identifier before the '(' matching the ')' at chunk[close]."""
    depth = 0
    for j in range(close, -1, -1):
        if chunk[j] == ')':
            depth += 1
        elif chunk[j] == '(':
            depth -= 1
            if depth == 0:
                m = re.search(r'(\w+)\s*$', chunk[:j])
                return m.group(1) if m else None
    return None


def strip_attributes(chunk, warn):
    while True:
        m = re.search(r'__attribute__\s*\(', chunk)
        if not m:
            return chunk
        end = match_paren(chunk, m.end() - 1)
        warn.add(chunk[m.start():end + 1])
        chunk = chunk[:m.start()] + chunk[end + 1:]


def convert(text, func):
    """Preprocessed source -> (base.c text, {C name: asm name}, C name of func)."""
    renames = {}
    warn = set()
    out = []
    chunks = top_level_chunks(text)
    # Renames first, so the target's C name is known.
    for k, ch in enumerate(chunks):
        m = re.search(r'\)\s*(?:asm|__asm__)\s*\(\s*"(\w+)"\s*\)', ch)
        if m and not re.match(r'(asm|__asm__)\b', ch):
            cname = name_before_paren(ch, m.start())
            renames[cname] = m.group(1)
            chunks[k] = ch[:m.start() + 1] + ch[m.end():]
    cfunc = next((c for c, a in renames.items() if a == func), func)
    found = False
    for ch in chunks:
        if re.match(r'(asm|__asm__)\s*\(', ch):
            continue  # INCLUDE_ASM and other top-level asm
        ch = strip_attributes(ch, warn)
        ch = re.sub(r'^(?:(?:static|inline|__inline__|__inline)\s+)+', '', ch)
        if ch.endswith('}') and not ch.startswith(('struct', 'union', 'enum', 'typedef')):
            brace = ch.index('{', len(ch) - len(ch.lstrip()))
            # The body is the last top-level {...}; find its '{' by matching from the end.
            depth = 0
            for j in range(len(ch) - 1, -1, -1):
                if ch[j] == '}':
                    depth += 1
                elif ch[j] == '{':
                    depth -= 1
                    if depth == 0:
                        brace = j
                        break
            head = ch[:brace].rstrip()
            name = name_before_paren(head, len(head) - 1)
            if name == cfunc:
                found = True
                out.append(ch)
            else:
                out.append(head + ';')
            continue
        if not ch.startswith('typedef'):
            depth = 0
            for j, c in enumerate(ch):
                if c in '({[':
                    depth += 1
                elif c in ')}]':
                    depth -= 1
                elif c == '=' and depth == 0:
                    ch = 'extern ' + ch[:j].rstrip() + ';'
                    break
        out.append(ch)
    if not found:
        sys.exit(f'{cfunc} is not defined in the source')
    for w in sorted(warn):
        print(f'note: dropped {w}')
    return '\n'.join(out) + '\n', renames, cfunc


def put(path, text):
    with open(str(path), 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)


def setup(ver, func, c_file, d):
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    pre = wsl(f'set -eo pipefail; cd {unix_path(ROOT)}; '
              f'cpp -nostdinc -undef -P -I include -I src -DVERSION_{ver} {unix_path(c_file)}')
    base, renames, cfunc = convert(pre, func)
    put(d / 'base.c', base)
    put(d / 'renames.sed', ''.join(f's/\\b{c}\\b/{a}/g\n' for c, a in renames.items()))
    # (Rewritten, not copied: a checkout with autocrlf gives it CRLFs, which bash rejects.)
    put(d / 'compile.sh', (HERE / 'compile.sh').read_text(encoding='utf-8').replace('\r\n', '\n'))
    absolute = (BUILD / ver / 'asm' / 'absolute.s').read_text(encoding='utf-8')
    put(d / 'abs.s', ''.join(l.strip() + '\n' for l in absolute.splitlines()
                             if l.strip().startswith('.set ')))
    # (No shell variables here: wsl.exe expands $names in its arguments itself.)
    tools = [wsl(f'command -v arm-none-eabi-{t} || echo ~/arm-binutils/usr/bin/arm-none-eabi-{t}').strip()
             for t in ('as', 'objdump')]
    put(d / 'tools.env', f'ARM_AS={tools[0]}\n')
    put(d / 'settings.toml',
        f'func_name = "{cfunc}"\ncompiler_type = "gcc"\n'
        f'objdump_command = "{tools[1]} -drz -M force-thumb"\n')
    asm = BUILD / ver / 'asm' / 'funcs' / f'{func}.s'
    wsl(f'set -e; cd {unix_path(d)}; chmod +x compile.sh; '
        f'{tools[0]} -mcpu=arm7tdmi -mthumb-interwork -o target.o {unix_path(asm)}; '
        f'./compile.sh base.c -o base.o')
    print(f'wrote {d.relative_to(ROOT)} ({cfunc}; renames: {renames or "none"})')
    return d


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('function')
    ap.add_argument('file', nargs='?')
    ap.add_argument('--comment', action='store_true',
                    help='use the near-miss comment above INCLUDE_ASM(function) in src/')
    ap.add_argument('--tag', help='use build/<ver>/permuter/<function>-<tag> (another starting point)')
    ap.add_argument('--run', action='store_true', help='run the permuter afterwards')
    ap.add_argument('-j', type=int, default=4, help='permuter threads')
    argv = sys.argv[1:]
    rest = argv[argv.index('--') + 1:] if '--' in argv else []
    args = ap.parse_args(argv[:len(argv) - len(rest) - (1 if rest or '--' in argv else 0)])
    ver, func = args.version, args.function
    d = BUILD / ver / 'permuter' / (func + (f'-{args.tag}' if args.tag else ''))
    if args.comment:
        src = next(p for p in (ROOT / 'src').rglob('*.c')
                   if f'INCLUDE_ASM({func});' in p.read_text(encoding='utf-8'))
        out = BUILD / ver / 'scratch'
        out.mkdir(parents=True, exist_ok=True)
        c_file = out / f'{func}.c'
        c_file.write_text(uncomment(src.read_text(encoding='utf-8'), func), encoding='utf-8')
        setup(ver, func, c_file, d)
    elif args.file:
        setup(ver, func, Path(args.file).resolve(), d)
    elif not (d / 'base.c').exists():
        sys.exit('no permuter dir yet: pass a file or --comment')
    if args.run:
        sys.exit(wsl(f'cd {PERMUTER} && python3 permuter.py {unix_path(d)} -j {args.j} '
                     + ' '.join(rest), capture=False))


if __name__ == '__main__':
    main()
