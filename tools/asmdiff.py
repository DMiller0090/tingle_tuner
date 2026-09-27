"""Side-by-side disassembly of the built program vs the original for one function.

    python tools/asmdiff.py GZLJ01 JoyComMain      # after a (mismatching) build
    python tools/asmdiff.py GZLJ01 0x02017EEC

The built function is found through build/<ver>/prog.map-independent means: its symbol in
build/<ver>/prog.elf (nm), so a size change earlier in the file doesn't misalign it.
Differing lines are marked with '*'.
"""
import argparse
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
    md.skipdata = True
    code = data[addr - base:addr - base + size]
    return [f'{i.mnemonic} {i.op_str}'.strip() for i in md.disasm(code, addr)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('function')
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
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else ''
        y = b[i] if i < len(b) else ''
        print(f'{"*" if x != y else " "} {x:40} {y}')


if __name__ == '__main__':
    main()
