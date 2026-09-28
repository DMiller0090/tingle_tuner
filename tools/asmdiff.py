"""Side-by-side disassembly of the built program vs the original for one function.

    python tools/asmdiff.py GZLJ01 JoyComMain      # after a (mismatching) build
    python tools/asmdiff.py GZLJ01 0x02017EEC

The built function is found through build/<ver>/prog.map-independent means: its symbol in
build/<ver>/prog.elf (nm), so a size change earlier in the file doesn't misalign it.
Differing lines are marked with '*'.
"""
import argparse
import difflib
import re
import subprocess

import capstone

from build import NM
from common import BUILD, versions


def functions(ver):
    out = []
    for line in (BUILD / ver / 'functions.txt').read_text().splitlines():
        a, size, mode, name = line.split()
        out.append((int(a, 16), int(size, 16), mode, name))
    return out


def built_symbols(ver):
    syms = {}
    for line in subprocess.run([NM, str(BUILD / ver / 'prog.elf')], capture_output=True,
                               text=True).stdout.splitlines():
        parts = line.split()
        if len(parts) == 3:
            syms[parts[2]] = int(parts[0], 16) & ~1
    return syms


def disasm(data, base, addr, size, mode):
    md = capstone.Cs(capstone.CS_ARCH_ARM,
                     capstone.CS_MODE_THUMB if mode == 'thumb' else capstone.CS_MODE_ARM)
    code = data[addr - base:addr - base + size]
    # One instruction at a time: capstone's own loop (even with skipdata) gives up at the
    # first literal pool word it can't decode, which silently truncated long functions.
    out = []
    step = 2 if mode == 'thumb' else 4
    off = 0
    while off < len(code):
        ins = next(md.disasm(code[off:off + 4], addr + off, count=1), None)
        if ins is None:
            out.append(f'.short {int.from_bytes(code[off:off + 2], "little"):#06x}'
                       if step == 2 else f'.word {int.from_bytes(code[off:off + 4], "little"):#010x}')
            off += step
            continue
        out.append(f'{ins.mnemonic} {ins.op_str}'.strip())
        off += ins.size
    return out


def mask(ins):
    """Hide what moves whenever code size changes: branch targets and pc-relative offsets."""
    op = ins.split(' ', 1)[0]
    if op in ('bl', 'blx') or (op.startswith('b') and re.match(r'b(\w\w)?(\.w)?$', op)):
        return op + ' ...'
    return re.sub(r'\[pc, #[^\]]*\]', '[pc, ...]', ins)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('function')
    ap.add_argument('-u', '--unified', action='store_true',
                    help='aligned diff (difflib) with branch/pool offsets masked')
    args = ap.parse_args()
    base = versions()[args.version]['prog_base']
    funcs = functions(args.version)
    if args.function.startswith('0x'):
        want = int(args.function, 16)
        func = next(f for f in funcs if f[0] == want)
    else:
        func = next(f for f in funcs if f[3] == args.function)
    addr, size, mode, name = func
    built_addr = built_symbols(args.version).get(name, addr)
    orig = (BUILD / args.version / 'prog.bin').read_bytes()
    built = (BUILD / args.version / 'prog_built.bin').read_bytes()
    a = disasm(orig, base, addr, size, mode)
    b = disasm(built, base, built_addr, size, mode)
    print(f'{name}: original {addr:#010x}, built {built_addr:#010x}')
    if args.unified:
        for line in difflib.unified_diff([mask(x) for x in a], [mask(y) for y in b],
                                         'original', 'built', n=2, lineterm=''):
            print(line)
        return
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else ''
        y = b[i] if i < len(b) else ''
        print(f'{"*" if x != y else " "} {x:40} {y}')


if __name__ == '__main__':
    main()
