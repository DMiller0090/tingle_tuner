# Combine-tracing agbcc

A copy of agbcc with debug output in `combine.c`, used to find out why combine drops or keeps
a sign extension (it matched `UpdateTargetCompass`). It prints, for the function named by
`DBG_FN`:

- `DBG_TC=1`: every `try_combine` / `can_combine_p` attempt and whether it succeeded;
- `DBG_REGS=45,46`: each value recorded for those pseudos (with its `reg_last_set_invalid`
  flag) and each `num_sign_bit_copies` lookup on them, including why the cached path failed.

Build it in WSL from a copy of pret's agbcc source:

```sh
cp -r ~/agbcc ~/agbcc_dbg_misc            # the agbcc checkout (gcc/, include/, libiberty/ ...)
cd ~/agbcc_dbg_misc/gcc
python3 /mnt/c/.../tools/agbcc_dbg/patch_combine.py   # reads combine.c, writes combine_dbg.c
cp combine_dbg.c combine.c && make                    # same make as agbcc's build.sh
```

Then `python tools/agbcc_dbg/run.py <ver> <func> file.c <regs>` (set `AGBCC_DBG` if the
binary is elsewhere). Pseudo numbers come from `tools/prio.py` or an `-dc` dump.
