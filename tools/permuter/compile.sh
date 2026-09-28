#!/bin/bash
# decomp-permuter compile script: compile.sh in.c -o out.o (runs in WSL/Linux).
# The input is already preprocessed (tools/permuter/perm.py writes base.c that way), so this
# is agbcc with build.py's flags, then the renames in renames.sed (functions declared
# `F_(...) asm("F")` in the source), then binutils `as`.
set -e
IN="$1"
OUT="$3"
HERE="$(cd "$(dirname "$0")" && pwd)"
AGBCC="${AGBCC:-$HOME/gba/agbcc}"
ARM_AS="${ARM_AS:-arm-none-eabi-as}"
[ -f "$HERE/tools.env" ] && . "$HERE/tools.env"
"$AGBCC/agbcc" -mthumb-interwork -Wimplicit -Wparentheses -O2 -fhex-asm "$IN" -o "$OUT.s" 2>/dev/null
if [ -s "$HERE/renames.sed" ]; then
    sed -i -E -f "$HERE/renames.sed" "$OUT.s"
fi
# abs.s: `.set` lines for the IWRAM/EWRAM/IO symbols, which the original's pools hold as
# plain numbers (build/<ver>/asm/absolute.s).
cat "$HERE/abs.s" "$OUT.s" | "$ARM_AS" -mcpu=arm7tdmi -mthumb-interwork -o "$OUT"
rm -f "$OUT.s"
