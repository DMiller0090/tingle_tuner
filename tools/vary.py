"""Try source variants of one function and score each against the original.

    python tools/vary.py GZLJ01 Effect8 build/GZLJ01/scratch/Effect8.c variants.py
    python tools/vary.py GZLJ01 Effect8 base.c variants.py name    # show one variant's diff

variants.py is Python that defines V = {name: [(old, new), ...]}: each variant applies its
replacements, in order, to the base C file from the function's definition on (so a helper
with the same text earlier in the file isn't touched). `old` matches with any whitespace, so
it can be copied without its indentation. Each variant is compiled with tools/scratch.py and
printed with its MATCH or masked diff size. Replacements can add declarations, change the
signature (e.g. to an asm label, `void F_(...) asm("F");`) or rewrite whole blocks.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

from common import BUILD, ROOT


def definition_start(src, func):
    """Offset of `func`'s definition (a prototype ends in `;` and doesn't match)."""
    m = re.search(r'^\w[\w \*]*\b' + re.escape(func) + r'_?\s*\([^;{]*\)\s*\n\{', src, re.M)
    if not m:
        sys.exit(f'no definition of {func} in the base file')
    return m.start()


def apply(src, func, reps):
    at = definition_start(src, func)
    head, body = src[:at], src[at:]
    for old, new in reps:
        pat = r'\s+'.join(re.escape(w) for w in old.split())
        m = re.search(pat, body)
        if not m:
            return None, old
        # The match starts at old's first token, after the line's indentation: drop new's.
        body = body[:m.start()] + new.lstrip(' \t') + body[m.end():]
    return head + body, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version')
    ap.add_argument('function')
    ap.add_argument('base', help='C file with the function (e.g. from scratch.py --comment)')
    ap.add_argument('variants', help='Python file defining V = {name: [(old, new), ...]}')
    ap.add_argument('only', nargs='?', help='run this variant and print its diff')
    args = ap.parse_args()

    src = Path(args.base).read_text(encoding='utf-8')
    ns = {}
    exec(Path(args.variants).read_text(encoding='utf-8'), ns)
    out = BUILD / args.version / 'scratch' / '_variant.c'
    out.parent.mkdir(parents=True, exist_ok=True)
    for name, reps in ns['V'].items():
        if args.only and name != args.only:
            continue
        text, missing = apply(src, args.function, reps)
        if text is None:
            print(f'{name}: not found: {missing[:60]!r}')
            continue
        out.write_text(text, encoding='utf-8')
        r = subprocess.run([sys.executable, str(ROOT / 'tools' / 'scratch.py'), args.version,
                            args.function, str(out)], capture_output=True, text=True)
        result = [l for l in r.stdout.splitlines() if l.startswith(args.function + ':')]
        print(f'{name}: ' + (result[0][len(args.function) + 2:] if result
                             else 'ERROR ' + (r.stdout + r.stderr)[-500:]))
        if args.only:
            print(r.stdout)


if __name__ == '__main__':
    main()
