"""Recursive-descent ARM/Thumb disassembler producing GAS source that rebuilds byte-for-byte.

    python tools/disasm.py GZLJ01

Reads build/<ver>/prog.bin (from extract.py) plus config/<ver>/symbols.txt (names and extra
function seeds), and writes build/<ver>/asm/prog.s and build/<ver>/functions.txt.

Code is discovered from the crt0 entry points by following branches, BL targets, Thumb
literals (odd pointers), agbcc switch tables, and odd pointers found in data. Everything
not reached is emitted as data. Instructions GAS would re-encode differently are emitted
as raw .2byte/.4byte with the disassembly in a comment (see build.py --fixup).
"""
import argparse
import re
import struct
from pathlib import Path

import capstone
from capstone import arm as carm

from common import BUILD, CONFIG, versions

ARM, THUMB = 'arm', 'thumb'
CC_AL = carm.ARM_CC_AL
PC_REL_RE = re.compile(r'\[pc(?:, #(-?(?:0x[0-9a-f]+|\d+)))?\]')
SYM_RE =re.compile(r'^\s*(\S+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;\s*(?://\s*(.*))?$')


def load_symbols(ver):
    """config/<ver>/symbols.txt: `name = 0xADDR; // thumb|arm|data [size:0x..]`."""
    syms = {}
    path = CONFIG / ver / 'symbols.txt'
    if path.exists():
        for line in path.read_text().splitlines():
            m = SYM_RE.match(line)
            if m:
                attrs = (m.group(3) or '').split()
                kind = attrs[0] if attrs else 'data'
                syms[int(m.group(2), 16)] = (m.group(1), kind)
    return syms


class Disassembler:
    def __init__(self, data, base, symbols):
        self.data = data
        self.base = base
        self.end = base + len(data)
        self.symbols = symbols
        self.cs = {}
        for mode, flag in ((ARM, capstone.CS_MODE_ARM), (THUMB, capstone.CS_MODE_THUMB)):
            md = capstone.Cs(capstone.CS_ARCH_ARM, flag)
            md.detail = True
            self.cs[mode] = md
        self.insns = {}        # addr -> capstone insn
        self.mode_at = {}      # addr -> mode (for code)
        self.funcs = {}        # addr -> mode
        self.labels = set()    # local branch targets
        self.pool = {}         # addr -> value (literal pool / jump table words)
        self.jumptables = {}   # table addr -> count
        self.data_refs = set() # even in-range values loaded by code (likely data labels)
        self.queue = []
        self.undefined_refs = []

    # ---- helpers -------------------------------------------------------------------------
    def in_range(self, addr):
        return self.base <= addr < self.end

    def word(self, addr):
        return struct.unpack_from('<I', self.data, addr - self.base)[0]

    def is_claimed(self, addr, size):
        return any((a in self.insns or a in self.pool_bytes) for a in range(addr, addr + size))

    def add_func(self, addr, mode):
        if not self.in_range(addr):
            return
        if addr not in self.funcs:
            self.funcs[addr] = mode
            self.queue.append((addr, mode))

    def add_label(self, addr, mode):
        if not self.in_range(addr):
            return
        self.labels.add(addr)
        if addr not in self.insns:
            self.queue.append((addr, mode))

    def add_pool(self, addr):
        if self.in_range(addr) and addr + 4 <= self.end:
            self.pool[addr] = self.word(addr)
            for a in range(addr, addr + 4):
                self.pool_bytes.add(a)
            return self.pool[addr]
        return None

    # ---- exploration ---------------------------------------------------------------------
    def run(self, seeds):
        self.pool_bytes = set()
        for addr, mode in seeds:
            self.add_func(addr, mode)
        while True:
            while self.queue:
                addr, mode = self.queue.pop()
                self.trace(addr, mode)
            if not self.seed_from_data() and not self.seed_from_holes():
                break

    def trace(self, addr, mode):
        md = self.cs[mode]
        align = 4 if mode == ARM else 2
        if addr % align:
            return
        history = []
        while self.in_range(addr) and addr not in self.insns and addr not in self.pool_bytes:
            code = self.data[addr - self.base:addr - self.base + 4]
            insn = next(md.disasm(code, addr, 1), None)
            if insn is None:
                return
            if mode == THUMB and insn.size == 4 and insn.mnemonic not in ('bl',):
                return  # Thumb-2 encoding: not ARMv4T code
            self.insns[addr] = insn
            self.mode_at[addr] = mode
            history.append(insn)
            if not self.step(insn, mode, history):
                return
            addr += insn.size

    def step(self, insn, mode, history):
        """Record effects of one instruction; return False if control flow stops here."""
        mn = insn.mnemonic
        ops = insn.op_str
        cond = insn.cc not in (CC_AL, 0)

        if mn == 'bl':
            self.add_func(int(ops.lstrip('#'), 16), mode)
            return True
        if mn.startswith('b') and insn.operands and insn.operands[0].type == carm.ARM_OP_IMM and mn != 'bic':
            target = insn.operands[0].imm
            self.add_label(target, mode)
            return cond
        if mn.startswith('ldr') and '[pc' in ops:
            lit = self.literal_addr(insn, mode)
            value = self.add_pool(lit)
            if value is not None:
                self.note_value(value)
                if ops.startswith('pc'):
                    return cond
            return True
        if mn.startswith('bx'):
            # `mov lr, pc` / `add lr, pc, #n` then bx: an indirect call that returns here.
            if len(history) >= 2 and history[-2].op_str.startswith('lr, pc'):
                return True
            return cond
        if mn.startswith('pop') or mn.startswith('ldm'):
            if 'pc' in ops:
                return cond
            return True
        if mn.startswith('mov') and ops.startswith('pc'):
            if mode == THUMB:
                self.jump_table(history)
            return cond
        if mn == 'svc' or mn.startswith('swi'):
            return True
        if mn.startswith('add') and ops.startswith('pc'):
            return cond
        return True

    def literal_addr(self, insn, mode):
        m = PC_REL_RE.search(insn.op_str)
        disp = int(m.group(1), 0) if m.group(1) else 0
        if mode == THUMB:
            return ((insn.address + 4) & ~3) + disp
        return insn.address + 8 + disp

    def note_value(self, value):
        if not self.in_range(value & ~1):
            return
        if value & 1:
            if self.plausible_thumb(value & ~1):
                self.add_func(value & ~1, THUMB)
        else:
            self.data_refs.add(value)

    def jump_table(self, history):
        """agbcc switch: cmp rX,#N / bhi / lsl / ldr rY,=table / add / ldr / mov pc."""
        table = count = None
        for insn in reversed(history[-10:-1]):
            if table is None and insn.mnemonic.startswith('ldr') and '[pc' in insn.op_str:
                table = self.pool.get(self.literal_addr(insn, THUMB))
            elif count is None and insn.mnemonic.startswith('cmp') and '#' in insn.op_str:
                count = int(insn.op_str.split('#')[-1], 0) + 1
        if table is None or not self.in_range(table):
            return
        func = max(a for a in self.funcs if a <= history[-1].address)
        n = 0
        while count is None or n < count:
            addr = table + n * 4
            if not self.in_range(addr + 3):
                break
            target = self.word(addr)
            if count is None and not (func <= target < func + 0x4000 and target % 2 == 0):
                break
            self.add_pool(addr)
            self.add_label(target, THUMB)
            n += 1
        self.jumptables[table] = n

    def seed_from_data(self):
        """Odd pointers into unclaimed program bytes found in data -> Thumb functions."""
        found = False
        for off in range(0, len(self.data) - 3, 4):
            addr = self.base + off
            if addr in self.insns or addr in self.pool_bytes:
                continue
            value = self.word(addr)
            target = value & ~1
            if (value & 1 and self.in_range(target) and target not in self.funcs
                    and target not in self.insns and self.looks_like_thumb_func(target)):
                self.add_func(target, THUMB)
                found = True
        return found

    def seed_from_holes(self):
        """agbcc .text is contiguous: an unclaimed hole between Thumb functions that decodes
        cleanly (after alignment padding) is an unreferenced function."""
        thumb = sorted(a for a, m in self.funcs.items() if m == THUMB)
        if not thumb:
            return False
        lo = thumb[0]
        hi = max(a + self.insns[a].size for a in self.insns if self.mode_at[a] == THUMB)
        found = False
        addr = lo
        while addr < hi:
            if addr in self.insns:
                addr += self.insns[addr].size
                continue
            if addr in self.pool_bytes or addr % 2:
                addr += 1
                continue
            hw = struct.unpack_from('<H', self.data, addr - self.base)[0]
            if hw == 0 or hw == 0x46C0:  # alignment padding / nop filler
                addr += 2
                continue
            mode = None
            if self.plausible_thumb(addr):
                mode = THUMB
            elif addr % 4 == 0 and self.plausible_arm(addr):
                mode = ARM
            if mode:
                self.add_func(addr, mode)
                found = True
                self.queue.pop()
                self.trace(addr, mode)
                while self.queue:
                    self.trace(*self.queue.pop())
            else:
                # Skip to the next claimed byte; this hole is data.
                while addr < hi and addr not in self.insns and addr not in self.pool_bytes:
                    addr += 2
        return found

    def plausible_arm(self, addr, limit=512):
        """Trial-decode ARM up to an unconditional return/branch."""
        md = self.cs[ARM]
        for _ in range(limit):
            if not self.in_range(addr + 3) or addr in self.pool_bytes or addr in self.insns:
                return False
            insn = next(md.disasm(self.data[addr - self.base:addr - self.base + 4], addr, 1), None)
            if insn is None or insn.cc == 0 or insn.mnemonic in ('nop', 'udf'):
                return False
            if insn.id in (carm.ARM_INS_BLX, carm.ARM_INS_MOVW, carm.ARM_INS_MOVT, carm.ARM_INS_LDRD,
                           carm.ARM_INS_STRD, carm.ARM_INS_CLZ):
                return False  # not ARMv4T
            mn, ops = insn.mnemonic, insn.op_str
            if insn.cc == CC_AL and (mn == 'bx' or mn == 'b' or ('pc' in ops and (mn.startswith('pop')
                                                                                    or mn.startswith('ldm')))):
                return True
            addr += 4
        return False

    def plausible_thumb(self, addr, limit=256):
        """Trial-decode: valid ARMv4T Thumb that reaches a return/branch before running off."""
        if addr in self.insns:
            return self.mode_at[addr] == THUMB
        md = self.cs[THUMB]
        for _ in range(limit):
            if not self.in_range(addr + 1) or addr in self.pool_bytes:
                return False
            code = self.data[addr - self.base:addr - self.base + 4]
            insn = next(md.disasm(code, addr, 1), None)
            if insn is None or (insn.size == 4 and insn.mnemonic != 'bl'):
                return False
            mn, ops = insn.mnemonic, insn.op_str
            if insn.cc in (CC_AL, 0) and (mn == 'bx' or (mn == 'pop' and 'pc' in ops)
                                           or (mn == 'b') or (mn.startswith('mov') and ops.startswith('pc'))):
                return True
            addr += insn.size
        return False

    def looks_like_thumb_func(self, addr):
        hw = struct.unpack_from('<H', self.data, addr - self.base)[0]
        return hw & 0xFE00 == 0xB400  # push {..., lr?}

    # ---- output --------------------------------------------------------------------------
    def name(self, addr):
        self._refs.add(addr)
        if addr in self.symbols:
            return self.symbols[addr][0]
        if addr in self.funcs:
            return f'{"sub" if self.funcs[addr] == THUMB else "arm"}_{addr:08X}'
        if addr in self.jumptables:
            return f'jtbl_{addr:08X}'
        if addr in self.insns:
            return f'.L{addr:08X}'
        return f'data_{addr:08X}'

    def data_label_addrs(self):
        out = set(a for a in self.data_refs if a not in self.insns)
        out |= set(self.jumptables)
        out |= set(a for a, (_, k) in self.symbols.items() if a not in self.insns)
        return out

    def render_insn(self, insn, mode):
        mn, ops = insn.mnemonic, insn.op_str
        if mn == 'bl' or (mn.startswith('b') and insn.operands and insn.operands[0].type == carm.ARM_OP_IMM
                          and mn != 'bic'):
            return f'{mn} {self.name(insn.operands[0].imm)}'
        if mn.startswith('ldr') and '[pc' in ops:
            lit = self.literal_addr(insn, mode)
            reg = ops.split(',')[0]
            return f'{mn} {reg}, {self.pool_label(lit)}'
        ops = ops.replace('apsr', 'cpsr')
        return f'{mn} {ops}'.rstrip()

    def pool_label(self, addr):
        return f'.Lpool_{addr:08X}'

    def emit(self, raw_addrs=frozenset()):
        """Return (items, absolutes, funcs): items are (addr, mode, text) lines in address order."""
        self._refs = set()
        defined = set()
        items = []
        data_labels = self.data_label_addrs()
        referenced_pools = set()
        for insn in self.insns.values():
            if insn.mnemonic.startswith('ldr') and '[pc' in insn.op_str:
                referenced_pools.add(self.literal_addr(insn, self.mode_at[insn.address]))
        funcs_list = []
        mode = ARM
        pending = []  # (start addr, bytes)

        def out(addr, text):
            items.append((addr, mode, text))

        def flush_bytes():
            if pending:
                start = pending[0][0]
                raw = [b for _, b in pending]
                for i in range(0, len(raw), 16):
                    out(start + i, '\t.byte ' + ', '.join(f'0x{b:02X}' for b in raw[i:i + 16]))
                pending.clear()

        addr = self.base
        while addr < self.end:
            if addr in self.insns:
                flush_bytes()
                insn = self.insns[addr]
                m = self.mode_at[addr]
                if m != mode:
                    mode = m
                    out(addr, '\t.thumb' if m == THUMB else '\t.arm')
                if addr in self.funcs:
                    name = self.name(addr)
                    funcs_list.append((addr, m, name))
                    out(addr, '')
                    out(addr, f'\t.global {name}')
                    if m == THUMB:
                        out(addr, '\t.thumb_func')
                    out(addr, f'{name}: @ {addr:08X}')
                    defined.add(addr)
                elif addr in self.labels or addr in self.symbols:
                    out(addr, f'{self.name(addr)}:')
                    defined.add(addr)
                raw = insn.bytes
                if addr in raw_addrs:
                    if len(raw) == 4 and m == ARM:
                        out(addr, f'\t.4byte 0x{struct.unpack("<I", raw)[0]:08X} @ {insn.mnemonic} {insn.op_str}')
                    else:
                        hws = struct.unpack(f'<{len(raw) // 2}H', raw)
                        out(addr, '\t.2byte ' + ', '.join(f'0x{h:04X}' for h in hws)
                            + f' @ {insn.mnemonic} {insn.op_str}')
                else:
                    out(addr, '\t' + self.render_insn(insn, m))
                addr += insn.size
                continue

            if addr in data_labels or addr in referenced_pools:
                flush_bytes()
                if addr in data_labels:
                    out(addr, f'\t.global {self.name(addr)}')
                    out(addr, f'{self.name(addr)}:')
                    defined.add(addr)
                if addr in referenced_pools:
                    out(addr, f'{self.pool_label(addr)}:')
            if (addr % 4 == 0 and addr + 4 <= self.end
                    and not any(a in self.insns for a in range(addr, addr + 4))
                    and not any(a in data_labels or a in referenced_pools for a in range(addr + 1, addr + 4))):
                flush_bytes()
                out(addr, '\t.4byte ' + self.symbolize(self.word(addr)))
                addr += 4
                continue
            pending.append((addr, self.data[addr - self.base]))
            addr += 1
        flush_bytes()
        out(self.end, '\t.global __prog_end')
        out(self.end, '__prog_end:')

        # Referenced addresses with no emitted label (outside the image, or mid-instruction).
        absolutes = []
        outside = {a for a in self.symbols if not self.in_range(a)}  # IWRAM/EWRAM vars, I/O
        for a in sorted((self._refs - defined) | outside):
            absolutes += [f'\t.global {self.name(a)}', f'\t.set {self.name(a)}, 0x{a:08X}']
        return items, absolutes, funcs_list

    def symbolize(self, value):
        target = value & ~1
        if value & 1 and self.funcs.get(target) == THUMB:
            return self.name(target)
        if not value & 1 and (value in self.funcs and self.funcs[value] == ARM
                              or value in self.labels and self.mode_at.get(value) == THUMB
                              or value in self.jumptables):
            return self.name(value)
        return f'0x{value:08X}'


ASM_HEADER = ['@ Generated by tools/disasm.py -- do not edit', '\t.syntax unified', '\t.cpu arm7tdmi',
              '\t.section .text', '']
SPLIT_RE = re.compile(r'^\s*(\S+)\s+(0x[0-9A-Fa-f]+)\s+(0x[0-9A-Fa-f]+)\s*$')


def load_splits(ver):
    """config/<ver>/splits.txt: `src/file.c 0xSTART 0xEND` -- address ranges built from C."""
    splits = []
    path = CONFIG / ver / 'splits.txt'
    if path.exists():
        for line in path.read_text().splitlines():
            line = line.split('#')[0]
            m = SPLIT_RE.match(line)
            if m:
                splits.append((int(m.group(2), 16), int(m.group(3), 16), m.group(1)))
    return sorted(splits)


def write_units(ver, items, absolutes, base, end):
    """Write asm segments around the C splits; return the link order [(kind, path)]."""
    asm_dir = BUILD / ver / 'asm'
    asm_dir.mkdir(parents=True, exist_ok=True)
    for old in asm_dir.glob('*.s'):
        old.unlink()
    splits = load_splits(ver)
    order = []
    cursor = base
    ranges = []
    for start, stop, src in splits:
        if cursor < start:
            ranges.append(('asm', cursor, start))
        ranges.append(('c', start, stop, src))
        cursor = stop
    ranges.append(('asm', cursor, end + 1))  # +1: include the __prog_end marker

    for r in ranges:
        if r[0] == 'c':
            order.append(('c', r[3]))
            continue
        _, lo, hi = r
        seg = [(a, m, t) for a, m, t in items if lo <= a < hi]
        if not seg:
            continue
        path = asm_dir / f'seg_{lo:08X}.s'
        first_mode = seg[0][1]
        body = ASM_HEADER + ['\t.thumb' if first_mode == THUMB else '\t.arm'] + [t for _, _, t in seg]
        path.write_text('\n'.join(body) + '\n')
        order.append(('asm', str(path.relative_to(BUILD / ver))))
    abs_path = asm_dir / 'absolute.s'
    abs_path.write_text('\n'.join(ASM_HEADER + absolutes) + '\n')
    order.append(('asm', str(abs_path.relative_to(BUILD / ver))))
    return order


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    args = ap.parse_args()

    cfg = versions()[args.version]
    out = BUILD / args.version
    data = (out / 'prog.bin').read_bytes()
    base = cfg['prog_base']
    symbols = load_symbols(args.version)

    d = Disassembler(data, base, symbols)
    seeds = [(base, ARM), (base + 0xC0, ARM)]
    seeds += [(a, k) for a, (_, k) in symbols.items() if k in (ARM, THUMB)]
    d.run(seeds)

    raw = set()
    raw_file = out / 'raw_insns.txt'
    if raw_file.exists():
        raw = {int(x, 16) for x in raw_file.read_text().split()}

    items, absolutes, funcs = d.emit(raw)
    order = write_units(args.version, items, absolutes, base, d.end)
    (out / 'link_order.txt').write_text(''.join(f'{k} {p}\n' for k, p in order))
    with open(out / 'functions.txt', 'w') as f:
        ends = [a for a, _, _ in funcs[1:]] + [d.end]
        for (addr, mode, name), nxt in zip(funcs, ends):
            f.write(f'{addr:08X} {nxt - addr:6X} {mode:5} {name}\n')

    code_bytes = sum(i.size for i in d.insns.values())
    print(f'{args.version}: {len(funcs)} functions ({sum(1 for _, m, _ in funcs if m == THUMB)} thumb), '
          f'{code_bytes:#x} code bytes, {len(d.pool)} pool words, {len(d.jumptables)} jump tables, '
          f'{len(raw)} raw insns, {sum(1 for k, _ in order if k == "c")} C units')


if __name__ == '__main__':
    main()
