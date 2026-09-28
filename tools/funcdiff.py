"""List the functions whose code differs between the original and the last build.

    python tools/funcdiff.py GZLJ01_demo

Unlike build.py, which stops at the first differing byte, this compares every function in
a C range by name (masked like asmdiff.py -u, skipping the original's literal pools and jump
tables), so it lists all of them even when earlier functions have a different size. Literal
pool contents are not compared: only build.py proves a match. Needs a build (prog.elf) first.
"""
import re
import sys

import capstone
from asmdiff import mask, built_symbols
from common import BUILD, versions
ver = sys.argv[1] if len(sys.argv) > 1 else sys.exit(__doc__)
base = versions()[ver]['prog_base']
want = (BUILD / ver / 'prog.bin').read_bytes()
got = (BUILD / ver / 'prog_built.bin').read_bytes()
built = built_symbols(ver)

def data_offsets(name, addr):
    """Offsets (from addr) of the original's pool/table words, from asm/funcs/<name>.s."""
    f = BUILD / ver / 'asm' / 'funcs' / f'{name}.s'
    out = set()
    if not f.exists(): return out
    cur = None
    for l in f.read_text().splitlines():
        m = re.match(r'(?:\.Lpool_|jtbl_)([0-9A-F]{8}):', l)
        if m: cur = int(m.group(1), 16); continue
        if cur is not None and l.startswith('\t.4byte'):
            out.update(range(cur - addr, cur - addr + 4)); cur += 4; continue
        if cur is not None and l.startswith('\t.2byte'):
            cur = None; continue
        if not l.startswith('	.global'):
            cur = None
    return out

def listing(data, addr, size, mode, skip):
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB if mode == 'thumb' else capstone.CS_MODE_ARM)
    code = data[addr - base:addr - base + size]
    out, off = [], 0
    while off < len(code):
        if off in skip: off += 1; continue
        ins = next(md.disasm(code[off:off + 4], addr + off, count=1), None)
        if ins is None: out.append('?'); off += 2; continue
        out.append(mask(f'{ins.mnemonic} {ins.op_str}'.strip())); off += ins.size
    return out

bad = []
for line in (BUILD / ver / 'functions.txt').read_text().splitlines():
    a, s, m, n = line.split(); a, s = int(a, 16), int(s, 16)
    b = built.get(n)
    if isinstance(b, tuple): b = b[0]
    if b is None: continue
    skip = data_offsets(n, a)
    if not skip and not (BUILD / ver / 'asm' / 'funcs' / f'{n}.s').exists(): continue
    w = listing(want, a, s, m, skip); g = listing(got, b, s, m, skip)
    if w != g:
        k = next((i for i in range(min(len(w), len(g))) if w[i] != g[i]), min(len(w), len(g)))
        bad.append((n, s, f'insn {k}/{len(w)}: {w[k] if k < len(w) else "-"} | {g[k] if k < len(g) else "-"}'))
for n, s, why in bad: print(f'{n:26} {s:#6x} {why}')
print(len(bad), 'differ')
