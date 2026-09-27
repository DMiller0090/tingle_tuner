"""Compile one function's C on its own and compare it with the original, without a full build.

    python tools/scratch.py GZLJ01 Effect8 scratch.c        # a file with the function in it
    python tools/scratch.py GZLJ01 Effect8 --comment        # the near-miss comment above
                                                             # INCLUDE_ASM(Effect8) in src/

The C file is compiled like build.py does, linked with the function at its original address
against the symbols of build/<ver>/prog.elf (minus the ones the file defines), and compared
byte for byte. Prints MATCH, or the masked aligned diff (as asmdiff.py -u) and its size.
--comment writes the file with the comment uncommented to build/<ver>/scratch/<name>.c, so
it can be edited and passed back in. Needs a build first (prog.elf).
"""
import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

from asmdiff import disasm, functions, mask
from build import AS, ASFLAGS, LD, NM, OBJCOPY, compile_c, run
from common import BUILD, ROOT, versions


def uncomment(src_text, name):
    """src_text with INCLUDE_ASM(name) replaced by the commented-out C just above it."""
    lines = src_text.split('\n')
    at = next(i for i, l in enumerate(lines) if l.strip() == f'INCLUDE_ASM({name});')
    if lines[at - 1].strip() == '*/':
        # A /* ... */ block (used when the code has macros with line continuations).
        start = max(i for i in range(at) if lines[i].strip() == '/*')
        return '\n'.join(lines[:start] + lines[start + 1:at - 1] + lines[at + 1:])
    start = at
    while start > 0 and lines[start - 1].startswith('//'):
        start -= 1
    block = [l[3:] if l.startswith('// ') else l[2:] for l in lines[start:at]]
    # Drop the prose: the code is the indented part.
    code_at = next(i for i, l in enumerate(block) if l.startswith('    '))
    code = [l[4:] for l in block[code_at:]]
    if not re.search(r'\b' + re.escape(name) + r'\s*\(', code[0]):
        print(f'warning: the comment has no definition of {name}; edit the scratch file')
    return '\n'.join(lines[:start] + code + lines[at + 1:])


def defined_symbols(obj):
    out = run([NM, '--defined-only', str(obj)]).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in 'TtDdRrBb':
            syms[parts[2]] = (int(parts[0], 16), parts[1])
    return syms


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('function')
    ap.add_argument('file', nargs='?')
    ap.add_argument('--comment', action='store_true',
                    help='use the near-miss comment above INCLUDE_ASM(function) in src/')
    ap.add_argument('--full', action='store_true', help='print the whole side-by-side listing')
    args = ap.parse_args()
    ver = args.version
    out = BUILD / ver / 'scratch'
    out.mkdir(parents=True, exist_ok=True)

    if args.comment:
        src = next(p for p in (ROOT / 'src').rglob('*.c')
                   if f'INCLUDE_ASM({args.function});' in p.read_text(encoding='utf-8'))
        c_file = out / f'{args.function}.c'
        c_file.write_text(uncomment(src.read_text(encoding='utf-8'), args.function),
                          encoding='utf-8')
        print(f'wrote {c_file.relative_to(ROOT)}')
    else:
        c_file = Path(args.file).resolve()

    # compile_c puts the output under build/<ver>/<path relative to ROOT>.
    obj = compile_c(ver, c_file.relative_to(ROOT))
    defs = defined_symbols(obj)
    name = args.function
    if name not in defs:
        sys.exit(f'{name} not defined in {c_file}')
    func = next(f for f in functions(ver) if f[3] == name)
    addr, size, mode, _ = func

    strip = out / 'strip.txt'
    strip.write_text('\n'.join(defs) + '\n')
    syms = out / 'syms.elf'
    run([OBJCOPY, f'--strip-symbols={strip}', str(BUILD / ver / 'prog.elf'), str(syms)])
    text_at = addr - defs[name][0]
    ld = out / 'scratch.ld'
    ld.write_text(f'SECTIONS {{\n  . = {text_at:#x};\n  .text : {{ *(.text) }}\n'
                  f'  .rodata : {{ *(.rodata) *(.rodata.*) }}\n'
                  f'  /DISCARD/ : {{ *(.ARM.attributes) *(.comment) }}\n}}\n')
    elf = out / 'scratch.elf'
    r = subprocess.run([LD, '-T', str(ld), '-R', str(syms), '-o', str(elf), str(obj)],
                       capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stdout + r.stderr)
    binf = out / 'scratch.bin'
    run([OBJCOPY, '-O', 'binary', '-j', '.text', str(elf), str(binf)])
    built = binf.read_bytes()
    base = versions()[ver]['prog_base']
    orig = (BUILD / ver / 'prog.bin').read_bytes()

    # The built function's size: up to the next symbol in the object's .text.
    starts = sorted(v for v, t in defs.values() if t in 'Tt')
    nxt = next((v for v in starts if v > defs[name][0]), len(built) + defs[name][0])
    bsize = nxt - defs[name][0]
    want = orig[addr - base:addr - base + size]
    got = built[defs[name][0]:defs[name][0] + bsize]
    if want == got:
        print(f'{name}: MATCH')
        return
    a = disasm(orig, base, addr, size, mode)
    b = disasm(built, text_at, addr, bsize, mode)
    diff = list(difflib.unified_diff([mask(x) for x in a], [mask(y) for y in b],
                                     'original', 'built', n=2, lineterm=''))
    ndiff = sum(1 for l in diff if l[:1] in '+-' and l[:3] not in ('+++', '---'))
    print(f'{name}: MISMATCH, size {bsize:#x} vs {size:#x}, {ndiff} masked diff lines')
    if args.full:
        for i in range(max(len(a), len(b))):
            x = a[i] if i < len(a) else ''
            y = b[i] if i < len(b) else ''
            print(f'{"*" if x != y else " "} {x:40} {y}')
    else:
        for line in diff:
            print(line)


if __name__ == '__main__':
    main()
