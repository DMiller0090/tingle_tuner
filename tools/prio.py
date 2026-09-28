"""Show how agbcc's global register allocator ranks one function's pseudos.

    python tools/prio.py GZLJ01 VoiceNoteOff file.c
    python tools/prio.py GZLJ01 VoiceNoteOff --comment     # the near-miss comment, as scratch.py
    python tools/prio.py GZLJ01 VoiceNoteOff file.c --rtl 43 --rtl 57

Compiles the file with `-dg -dl` (build/<ver>/scratch/prio/) and prints, for the function, the
pseudos global-alloc sorts ("Registers to be allocated in sorted order") with their refs,
live length, priority and the hard register each ended in. global.c's priority is
floor_log2(refs) * refs / live_length (x10000, x size); refs already count loop uses double
per loop depth. Allocation goes in priority order and ties keep the lower pseudo first, so a
register swap between two values usually means their priorities compare the other way round
from the original: change refs (extra uses, loop placement) or live length (where the value
is set, a wider/narrower temp) to flip them. `*` marks pseudos global-alloc allocated
(the rest were placed by local-alloc inside one block). --rtl N prints the insns that
mention pseudo N, to tell which C variable it is.
"""
import argparse
import re
import subprocess
from pathlib import Path

from build import AGBCC_DIR, CFLAGS, WSL_DISTRO, unix_path
from common import BUILD, ROOT, version_defines, versions
from scratch import uncomment


def floor_log2(n):
    return n.bit_length() - 1 if n > 0 else -1


def section(text, name):
    """The part of a dump file for function name."""
    m = re.search(r'^;; Function ' + re.escape(name) + r'\n', text, re.M)
    if not m:
        # `T F_(...) asm("F")` definitions: the dump uses the C name.
        m = re.search(r'^;; Function ' + re.escape(name) + r'_\n', text, re.M)
    if not m:
        raise SystemExit(f'{name} not in the dump')
    end = text.find('\n;; Function ', m.end())
    return text[m.end():end if end >= 0 else len(text)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()))
    ap.add_argument('function')
    ap.add_argument('file', nargs='?')
    ap.add_argument('--comment', action='store_true')
    ap.add_argument('--rtl', type=int, action='append', default=[],
                    help='print the insns (after local-alloc) that mention this pseudo')
    args = ap.parse_args()
    ver, name = args.version, args.function
    out = BUILD / ver / 'scratch' / 'prio'
    out.mkdir(parents=True, exist_ok=True)

    if args.comment:
        src = next(p for p in (ROOT / 'src').rglob('*.c')
                   if f'INCLUDE_ASM({name});' in p.read_text(encoding='utf-8'))
        c_file = out / f'{name}.c'
        c_file.write_text(uncomment(src.read_text(encoding='utf-8'), name), encoding='utf-8')
    else:
        c_file = Path(args.file).resolve()

    i_file = out / 'x.i'
    script = (f'set -eo pipefail; cd {unix_path(ROOT)}; '
              f'cpp -nostdinc -undef -P -I include -I src {version_defines(ver)} {unix_path(c_file)} '
              f'> {unix_path(i_file)}; cd {unix_path(out)}; '
              f'{AGBCC_DIR}/agbcc {CFLAGS} -dg -dl x.i -o x.s')
    import os
    cmd = ['bash', '-c', script] if os.name != 'nt' else ['wsl', '-d', WSL_DISTRO, '--', 'bash', '-c', script]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(r.stdout + r.stderr)

    greg = section((out / 'x.i.greg').read_text(encoding='utf-8', errors='replace'), name)
    lreg = section((out / 'x.i.lreg').read_text(encoding='utf-8', errors='replace'), name)

    info = {}
    for m in re.finditer(r'^Register (\d+) used (\d+) times across (\d+) insns(.*)$', lreg, re.M):
        notes = [n.strip() for n in m.group(4).split(';')]
        notes = [n for n in notes if n and not n.startswith('set ') and n != '.'
                 and not n.startswith('in block')]
        info[int(m.group(1))] = ', '.join(notes).rstrip('.')
    sorted_regs = [(int(a), int(b), int(c), int(d)) for a, b, c, d in re.findall(
        r'^Register (\d+), refs = (\d+), live_length = (\d+), size = (\d+)', greg, re.M)]
    m = re.search(r'^;; \d+ regs to allocate:(.*)$', greg, re.M)
    global_set = {int(x) for x in m.group(1).split()} if m else set()
    disp = {}
    m = re.search(r';; Register dispositions:\n(.*?)\n\n', greg, re.S)
    if m:
        for a, b in re.findall(r'(\d+) in (\d+)', m.group(1)):
            disp[int(a)] = int(b)

    print(f'{name}: {len(sorted_regs)} pseudos for global-alloc, {len(global_set)} allocated by it '
          f'(* = allocated by global-alloc; rank = its sort position)')
    print(f'{"":1} {"rank":>4} {"pseudo":>6} {"refs":>5} {"live":>5} {"priority":>9} {"hard":>5}  notes')
    for rank, (reg, refs, live, size) in enumerate(sorted_regs):
        prio = floor_log2(refs) * refs * 10000 * size // live if live else 0
        mark = '*' if reg in global_set else ' '
        hard = disp.get(reg)
        hard = f'r{hard}' if hard is not None and hard >= 0 else 'mem'
        print(f'{mark} {rank:>4} {reg:>6} {refs:>5} {live:>5} {prio:>9} {hard:>5}  {info.get(reg, "")}')

    for reg in args.rtl:
        print(f'\n-- insns mentioning pseudo {reg} (after local-alloc):')
        for insn in re.split(r'\n(?=\((?:insn|jump_insn|call_insn) )', lreg):
            if not insn.startswith('(') or not re.search(r'\(reg(?:/[a-z]+)*:\w+ ' + str(reg) + r'\)', insn):
                continue
            print(re.sub(r'\n\s*', ' ', insn.split('\n\n')[0])[:300])


if __name__ == '__main__':
    main()
