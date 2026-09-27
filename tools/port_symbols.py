"""Port names from one version's symbols.txt to another by matching functions.

    python tools/port_symbols.py GZLJ01 GZLE01            # show what would change
    python tools/port_symbols.py GZLJ01 GZLE01 --write    # rewrite config/GZLE01/symbols.txt

Functions (build/<ver>/functions.txt) are matched in address order by their mnemonic
sequence (difflib over per-function hashes; unmatched runs are paired in order). A data
symbol is ported by finding a function whose literal pool holds its address, then reading
the literal at the same place in the matching function. Anything neither finds (IWRAM
variables, I/O) keeps its address; IWRAM layout is identical in every version so far. EWRAM
below the program (link buffers) moves with prog_base.

Run tools/build.py (or disasm.py) for both versions first so functions.txt is current.
"""
import argparse
import difflib
import hashlib
import struct

import capstone

from common import BUILD, CONFIG, versions
from disasm import SYM_RE, load_symbols

EWRAM_START = 0x02000000
MODES = {'arm': capstone.CS_MODE_ARM, 'thumb': capstone.CS_MODE_THUMB}


class Image:
    def __init__(self, ver, skip=()):
        self.ver = ver
        self.base = versions()[ver]['prog_base']
        self.data = (BUILD / ver / 'prog.bin').read_bytes()
        self.end = self.base + len(self.data)
        self.funcs = []  # (addr, size, mode)
        for line in (BUILD / ver / 'functions.txt').read_text().splitlines():
            a, size, mode, _ = line.split()
            if int(a, 16) not in skip:
                self.funcs.append((int(a, 16), int(size, 16), mode))
        self.sigs = [self.signature(f) for f in self.funcs]

    def signature(self, func):
        addr, size, mode = func
        md = capstone.Cs(capstone.CS_ARCH_ARM, MODES[mode])
        md.skipdata = True
        code = self.data[addr - self.base:addr - self.base + size]
        return [i.mnemonic for i in md.disasm(code, addr)]

    def words(self, func):
        """(offset in function, word) for every aligned word in the function's range."""
        addr, size, _ = func
        start = (addr + 3) & ~3
        return [(a - addr, struct.unpack_from('<I', self.data, a - self.base)[0])
                for a in range(start, addr + size - 3, 4)]


def match_functions(src, dst):
    ha = [hashlib.md5(' '.join(s).encode()).hexdigest() for s in src.sigs]
    hb = [hashlib.md5(' '.join(s).encode()).hexdigest() for s in dst.sigs]
    pairs = {}
    ops = difflib.SequenceMatcher(None, ha, hb, autojunk=False).get_opcodes()
    for tag, i1, i2, j1, j2 in ops:
        if tag in ('equal', 'replace'):
            for k in range(min(i2 - i1, j2 - j1)):
                pairs[i1 + k] = j1 + k
    return pairs


def dst_only(dst_ver):
    """Names in the destination's `// <ver> only` block: {addr: name}."""
    text = (CONFIG / dst_ver / 'symbols.txt').read_text().splitlines()
    marker = f'// {dst_ver} only'
    out = {}
    if marker in text:
        for line in text[text.index(marker) + 1:]:
            m = SYM_RE.match(line)
            if m:
                out[int(m.group(2), 16)] = m.group(1)
    return out


def port(src_ver, dst_ver):
    # Functions only the destination has (named in its file) are left out of the matching,
    # so the pairing in order doesn't give their names' neighbours the wrong function.
    src, dst = Image(src_ver), Image(dst_ver, skip=dst_only(dst_ver))
    pairs = match_functions(src, dst)
    src_index = {f[0]: i for i, f in enumerate(src.funcs)}
    lines = []
    path = CONFIG / src_ver / 'symbols.txt'
    for line in path.read_text().splitlines():
        m = SYM_RE.match(line)
        if not m:
            continue
        name, addr = m.group(1), int(m.group(2), 16)
        attrs = m.group(3) or 'data'
        new = None
        if addr in src_index:
            j = pairs.get(src_index[addr])
            if j is not None:
                new = dst.funcs[j][0]
        elif src.base <= addr < src.end:
            new = port_data(src, dst, pairs, addr)
        elif EWRAM_START <= addr < src.base:
            # EWRAM buffers below the program keep their offset from prog_base. (Pool
            # voting is unreliable here: small EWRAM values also turn up as other data.)
            new = addr - src.base + dst.base
        else:
            new = port_data(src, dst, pairs, addr, outside=True)
            if new is None:
                new = addr
        lines.append((name, new, attrs, addr))
    return lines


def port_data(src, dst, pairs, addr, outside=False):
    """Find `addr` (or addr+off for small off) in a pool and read the matching literal."""
    votes = {}  # delta -> {candidate: count}; exact references win over nearby ones
    for i, func in enumerate(src.funcs):
        j = pairs.get(i)
        if j is None:
            continue
        dwords = dict(dst.words(dst.funcs[j]))
        for off, w in src.words(func):
            delta = addr - w
            if 0 <= delta < 0x100 and (not outside or delta == 0) and off in dwords:
                cand = dwords[off] + delta
                # A reshaped function can hold anything at the same pool offset.
                if not outside and not dst.base <= cand < dst.end:
                    continue
                bucket = votes.setdefault(delta, {})
                bucket[cand] = bucket.get(cand, 0) + 1
    if not votes:
        # Not referenced: data embedded in a function (e.g. the ROM header in crt0).
        for i, (a, size, _) in enumerate(src.funcs):
            if a <= addr < a + size and i in pairs:
                return dst.funcs[pairs[i]][0] + addr - a
        return None
    bucket = votes[min(votes)]
    return max(bucket, key=bucket.get)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src', choices=sorted(versions()))
    ap.add_argument('dst', choices=sorted(versions()))
    ap.add_argument('--write', action='store_true')
    args = ap.parse_args()

    ported = port(args.src, args.dst)
    existing = load_symbols(args.dst)
    by_name = {n: a for a, (n, _) in existing.items()}
    out = [f'// {args.dst} symbols: name = 0xADDR; // arm|thumb|data',
           f'// Ported from {args.src} by tools/port_symbols.py; edit {args.src} and re-port.']
    for name, new, attrs, old in ported:
        if new is None:
            print(f'  unmatched: {name} ({old:#010x})')
            continue
        if by_name.get(name) != new:
            print(f'  {name}: {by_name.get(name, 0):#010x} -> {new:#010x}')
        out.append(f'{name} = 0x{new:08X}; // {attrs}')
    # Names only the destination has (version-specific code) are kept as they are, with
    # the comments above them.
    ported_names = {p[0] for p in ported}
    extra = []
    marker = f'// {args.dst} only'
    text = (CONFIG / args.dst / 'symbols.txt').read_text().splitlines()
    if marker in text:
        for line in text[text.index(marker) + 1:]:
            m = SYM_RE.match(line)
            if m is None or m.group(1) not in ported_names:
                extra.append(line)
    listed = {SYM_RE.match(l).group(1) for l in extra if SYM_RE.match(l)}
    # Other leftovers are stale when a ported name now has their address (a renamed source
    # symbol); anything else is kept.
    taken = {p[1] for p in ported}
    extra += [f'{n} = 0x{a:08X}; // {k}' for a, (n, k) in sorted(existing.items())
              if n not in ported_names and n not in listed and a not in taken]
    if extra:
        out.append(marker)
        out += extra
    if args.write:
        (CONFIG / args.dst / 'symbols.txt').write_text('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
