"""Compile one C file with the combine-tracing agbcc and print its trace.

    python tools/agbcc_dbg/run.py GZLJ01 UpdateTargetCompass file.c 45,46

REGS is a comma-separated list of pseudo numbers (see prio.py or `-dc` dumps) whose recorded
values and sign-bit-copy lookups are printed; try_combine/can_combine_p results are printed
for every insn of FUNC. The compiler is $AGBCC_DBG in WSL (default
~/agbcc_dbg_misc/gcc/agbcc); see README.md here for building it.
"""
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from build import CFLAGS, WSL_DISTRO, unix_path  # noqa: E402

AGBCC_DBG = os.environ.get('AGBCC_DBG', '~/agbcc_dbg_misc/gcc/agbcc')


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    ver, name, cfile, regs = sys.argv[1:5]
    out = ROOT / 'build' / ver / 'scratch' / 'dbg'
    out.mkdir(parents=True, exist_ok=True)
    script = (f'set -eo pipefail; cd {unix_path(ROOT)}; '
              f'cpp -nostdinc -undef -P -I include -I src -DVERSION_{ver} '
              f'{unix_path(Path(cfile).resolve())} > {unix_path(out / "x.i")}; '
              f'cd {unix_path(out)}; '
              f'DBG_TC=1 DBG_FN={name} DBG_REGS={regs} {AGBCC_DBG} {CFLAGS} x.i -o x.s')
    r = subprocess.run(['wsl', '-d', WSL_DISTRO, '--', 'bash', '-c', script],
                       capture_output=True, text=True)
    print(r.stdout + r.stderr)


if __name__ == '__main__':
    main()
