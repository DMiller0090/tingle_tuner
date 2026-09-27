"""Decompilation progress: how much of the program's code is built from C.

    python tools/progress.py           # every extracted version
    python tools/progress.py GZLJ01

A function counts as C when it lies in a C range of config/<ver>/splits.txt and its file
doesn't pull it in with INCLUDE_ASM. Sizes come from build/<ver>/functions.txt (run
build.py first), so they include each function's literal pool; .text ends at
gIntrTableInit. Hand-written asm modules (crt0, the ARM mixer, libagbsyscall, libgcc) are
counted in the total too.
"""
import argparse
import re
from pathlib import Path

from common import BUILD, CONFIG, ROOT, versions


def c_ranges(ver):
    out = []
    for line in (CONFIG / ver / 'splits.txt').read_text().splitlines():
        parts = line.split('#', 1)[0].split()
        if len(parts) == 3:
            out.append((parts[0], int(parts[1], 16), int(parts[2], 16)))
    return out


def text_end(ver):
    # .text ends where .rodata starts, and gIntrTableInit is .rodata's first object.
    m = re.search(r'^gIntrTableInit = (0x[0-9A-Fa-f]+);',
                  (CONFIG / ver / 'symbols.txt').read_text(), re.M)
    return int(m.group(1), 16)


def progress(ver):
    end = text_end(ver)
    funcs = []
    for line in (BUILD / ver / 'functions.txt').read_text().splitlines():
        a, size, mode, name = line.split()
        a = int(a, 16)
        # The last function's size runs to the end of the image.
        funcs.append((a, min(int(size, 16), end - a), name))
    asm_names = {}
    for src, _, _ in c_ranges(ver):
        # agbcc's output for this version: INCLUDE_ASM inside #ifdef VERSION_* counts only
        # where it survives the preprocessor.
        built = BUILD / ver / Path(src).with_suffix('.s')
        if built.exists():
            text = built.read_text(encoding='utf-8', errors='replace')
            asm_names[src] = set(re.findall(r'\.include \\?"funcs/(\w+)\.s', text))
        else:
            text = (ROOT / src).read_text(encoding='utf-8')
            asm_names[src] = set(re.findall(r'^INCLUDE_ASM\((\w+)\);', text, re.M))
    total = sum(size for _, size, _ in funcs)
    c_bytes = c_funcs = 0
    for addr, size, name in funcs:
        for src, start, stop in c_ranges(ver):
            if start <= addr < stop and name not in asm_names[src]:
                c_bytes += size
                c_funcs += 1
                break
    return c_bytes, total, c_funcs, len(funcs)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', nargs='?')
    args = ap.parse_args()
    vers = [args.version] if args.version else sorted(versions())
    for ver in vers:
        if not (BUILD / ver / 'functions.txt').exists():
            print(f'{ver}: not built')
            continue
        c_bytes, total, c_funcs, n = progress(ver)
        print(f'{ver}: {c_bytes:#x} / {total:#x} bytes in C ({100 * c_bytes / total:.2f}%), '
              f'{c_funcs} / {n} functions')


if __name__ == '__main__':
    main()
