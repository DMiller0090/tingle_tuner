"""Build and verify a client: assemble/compile, link, repack, compare SHA-1s.

    python tools/build.py GZLJ01
    python tools/build.py all
    python tools/build.py GZLJ01 --fixup    # pin instructions GAS re-encodes as raw bytes

Steps: disasm.py writes build/<ver>/asm/seg_*.s around the C ranges listed in
config/<ver>/splits.txt; C files are compiled with agbcc (pret's GCC 2.9-arm-000512
rebuild -- the compiler the game was built with) under WSL; everything is linked at
prog_base in address order, compared to the original program, LZ77-packed with
src/packer.s, and compared to the retail client.
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

import lz77
from common import BUILD, ROOT, orig_path, sha1, versions


def _find_devkitarm():
    # $DEVKITARM is often an msys path (/opt/devkitpro/...) that native Python can't use.
    for cand in (os.environ.get('DEVKITARM'), 'C:/devkitPro/devkitARM', '/opt/devkitpro/devkitARM'):
        if cand and (Path(cand) / 'bin').is_dir():
            return Path(cand)
    sys.exit('devkitARM not found; set DEVKITARM')


DEVKITARM = _find_devkitarm()
EXE = '.exe' if os.name == 'nt' else ''
AS = str(DEVKITARM / 'bin' / f'arm-none-eabi-as{EXE}')
LD = str(DEVKITARM / 'bin' / f'arm-none-eabi-ld{EXE}')
OBJCOPY = str(DEVKITARM / 'bin' / f'arm-none-eabi-objcopy{EXE}')
NM = str(DEVKITARM / 'bin' / f'arm-none-eabi-nm{EXE}')
ASFLAGS = ['-mcpu=arm7tdmi', '-mthumb-interwork']

# agbcc is a Linux binary: run it natively or through WSL. AGBCC = dir containing agbcc.
AGBCC_DIR = os.environ.get('AGBCC', '~/gba/agbcc')
CFLAGS = '-mthumb-interwork -Wimplicit -Wparentheses -O2 -fhex-asm'
WSL_DISTRO = os.environ.get('WSL_DISTRO', 'Ubuntu')


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode:
        sys.stderr.write(r.stdout + r.stderr)
        sys.exit(f'failed: {" ".join(map(str, cmd))}')
    return r


def unix_path(p):
    """Path as seen by the shell that runs agbcc."""
    p = Path(p).resolve()
    if os.name != 'nt':
        return str(p)
    drive, rest = p.drive, p.as_posix()[len(p.drive):]
    return f'/mnt/{drive[0].lower()}{rest}'


def compile_c(ver, src):
    """src/foo.c -> build/<ver>/src/foo.o via cpp | agbcc | as."""
    src = ROOT / src
    rel = src.relative_to(ROOT).with_suffix('')
    s_file = BUILD / ver / rel.with_suffix('.s')
    o_file = BUILD / ver / rel.with_suffix('.o')
    s_file.parent.mkdir(parents=True, exist_ok=True)
    defs = f'-DVERSION_{ver}'
    script = (f'set -eo pipefail; cd {unix_path(ROOT)}; '
              f'cpp -nostdinc -undef -P -I include -I src {defs} {unix_path(src)} '
              f'| {AGBCC_DIR}/agbcc {CFLAGS} -o {unix_path(s_file)}')
    cmd = ['bash', '-c', script] if os.name != 'nt' else ['wsl', '-d', WSL_DISTRO, '--', 'bash', '-c', script]
    run(cmd)
    run([AS] + ASFLAGS + ['-I', str(BUILD / ver / 'asm'), '-o', str(o_file), str(s_file)])
    return o_file


ALL_SECTIONS = '.text .rodata .rodata.*'


def link_bin(objs, base, out_stem):
    """objs: object paths, or (path, input sections) to place parts of one object apart."""
    objs = [o if isinstance(o, tuple) else (o, ALL_SECTIONS) for o in objs]
    ld_script = out_stem.with_suffix('.ld')
    ld_script.write_text(
        f'SECTIONS {{\n  . = {base:#x};\n  .text : {{\n'
        + ''.join(f'    "{Path(o).as_posix()}"({secs})\n' for o, secs in objs)
        + '  }\n  /DISCARD/ : { *(.ARM.attributes) *(.comment) }\n}\n')
    elf = out_stem.with_suffix('.elf')
    files = list(dict.fromkeys(str(o) for o, _ in objs))
    run([LD, '-T', str(ld_script), '-o', str(elf), '-Map', str(out_stem.with_suffix('.map'))] + files)
    binf = Path(str(out_stem) + '_built.bin')
    run([OBJCOPY, '-O', 'binary', str(elf), str(binf)])
    data = binf.read_bytes()
    # Section sizes are padded to 4; trim to the image's real end marker if present.
    for line in run([NM, str(elf)]).stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == '__prog_end':
            data = data[:int(parts[0], 16) - base]
            binf.write_bytes(data)
    return data


def build_prog(ver):
    cfg = versions()[ver]
    out = BUILD / ver
    run([sys.executable, str(ROOT / 'tools' / 'disasm.py'), ver])
    order = [line.split(maxsplit=1) for line in (out / 'link_order.txt').read_text().splitlines()]
    split_rodata = {path for kind, path in order if kind == 'c_rodata'}
    c_objs = {}
    objs = []
    for kind, path in order:
        if kind in ('c', 'c_rodata'):
            if path not in c_objs:
                c_objs[path] = compile_c(ver, path)
            if kind == 'c_rodata':
                objs.append((c_objs[path], '.rodata .rodata.*'))
            elif path in split_rodata:
                objs.append((c_objs[path], '.text'))
            else:
                objs.append(c_objs[path])
        else:
            src = out / path
            obj = src.with_suffix('.o')
            run([AS] + ASFLAGS + ['-o', str(obj), str(src)])
            objs.append(obj)
    return link_bin(objs, cfg['prog_base'], out / 'prog')


def first_diff(got, want):
    n = min(len(got), len(want))
    return next((i for i in range(n) if got[i] != want[i]), n if len(got) != len(want) else None)


def function_at(ver, addr):
    best = None
    for line in (BUILD / ver / 'functions.txt').read_text().splitlines():
        a, _, _, name = line.split()
        if int(a, 16) <= addr:
            best = (int(a, 16), name)
    return best


def fixup(ver, got, want):
    """Pin the first differing asm instruction as raw bytes (GAS chose another encoding)."""
    base = versions()[ver]['prog_base']
    off = first_diff(got, want)
    raw_file = BUILD / ver / 'raw_insns.txt'
    old = set(raw_file.read_text().split()) if raw_file.exists() else set()
    addr = f'{base + (off & ~1):08X}'
    if addr in old:
        return False
    raw_file.write_text('\n'.join(sorted(old | {addr})) + '\n')
    return True


def pack(ver, prog):
    """Rebuild the outer client from src/packer.s + LZ77(prog) + raw tail."""
    cfg = versions()[ver]
    out = BUILD / ver
    orig = orig_path(ver).read_bytes()
    (out / 'header.bin').write_bytes(orig[0x04:0xC0])
    (out / 'prog.lz').write_bytes(lz77.compress(prog))
    obj = out / 'packer.o'
    run([AS] + ASFLAGS + ['-I', str(out), '--defsym', f'PROG_BASE={cfg["prog_base"]:#x}',
                          '--defsym', f'TAIL_DEST={cfg["tail_dest"]:#x}',
                          '-o', str(obj), str(ROOT / 'src' / 'packer.s')])
    return link_bin([obj], 0x02000000, out / 'client')


def build(ver, do_fixup):
    want_prog = (BUILD / ver / 'prog.bin').read_bytes()
    for _ in range(64):
        prog = build_prog(ver)
        if prog == want_prog or not do_fixup or not fixup(ver, prog, want_prog):
            break
    ok_prog = prog == want_prog
    if not ok_prog:
        off = first_diff(prog, want_prog)
        base = versions()[ver]['prog_base']
        fn = function_at(ver, base + off)
        print(f'{ver}: prog differs first at {base + off:#010x}'
              + (f' ({fn[1]}+{base + off - fn[0]:#x})' if fn else '')
              + f'; size {len(prog):#x} vs {len(want_prog):#x}')
    client = pack(ver, prog)
    name = Path(versions()[ver]['disc_path']).name
    (BUILD / ver / name).write_bytes(client)
    ok = sha1(client) == versions()[ver]['sha1']
    print(f'{ver}: prog {"OK" if ok_prog else "MISMATCH"} {sha1(prog)}; '
          f'{name} {"OK" if ok else "MISMATCH"} {sha1(client)}')
    return ok_prog and ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('version', choices=sorted(versions()) + ['all'])
    ap.add_argument('--fixup', action='store_true')
    args = ap.parse_args()
    vers = sorted(versions()) if args.version == 'all' else [args.version]
    results = [build(v, args.fixup) for v in vers]
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
