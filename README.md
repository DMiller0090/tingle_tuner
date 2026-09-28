# Tingle Tuner decompilation

Matching decompilation of the GBA program that *The Wind Waker* JOY-boots onto a linked Game
Boy Advance (the Tingle Tuner): `/res/Gba/client*.bin` on the disc. The GameCube side of the
link lives in the main TWW decomp (`d_a_agb.cpp`, `m_Do_gba_com.cpp`, `JUTGba.cpp`,
`dolphin/gba/`).

| Version     | File             | Program base | Status |
|-------------|------------------|--------------|--------|
| `GZLJ01`    | `client.bin`     | `0x02014800` | matching (asm + C) |
| `GZLE01`    | `client_u.bin`   | `0x0201B000` | matching (asm + C) |
| `GZLE01_ud` | `client_ud.bin`  | `0x0201B000` | matching (asm + C); second USA build, unused by the game code |
| `GZLJ01_demo` | `client.bin` of the JP store demo | `0x02014800` | matching (asm + C); earlier JP build, see docs/layout.md |
| `GZLP01_<n>` | `client_<n>.bin` | `0x0201B000`, German `0x0201CB00`, French `0x0201BE00` | matching (asm + C); PAL, n = 0 English, 1 German, 2 French, 3 Spanish, 4 Italian |
| `GZLP01_<n>d` | `client_<n>d.bin` | as `GZLP01_<n>` | matching (asm + C); second PAL builds, unused like `client_ud.bin` |

## What the binary is

Each client is a GBA multiboot image with two layers:

1. **Packer** (`src/packer.s`, identical code in every version): standard header, a JOY bus
   handshake at `0xE0` (sends the game code, waits for the GC to echo it), then `CpuFastSet` copies
   the raw tail block (the sound bank) up to `tail_dest`, `LZ77UnCompWram` unpacks the program to `prog_base`, and it
   jumps there.
2. **Program**: a normal AGB SDK multiboot image. Stock SDK `crt0.s` (ARM), then mostly Thumb
   code compiled by **GCC 2.9-arm-000512** (its DWARF leftovers name the compiler), plus
   newlib, Nintendo's `MultiSioSync` library (version `010316`), and a few ARM PCM-mixing
   routines. Not using the m4a sound engine.

The LZ77 layer is reproduced exactly: Nintendo's encoder is plain greedy (longest match,
nearest on ties, 4 KiB window), see `tools/lz77.py`.

## Setup

Requirements: Python 3.7+ with `capstone` and `pyyaml`, devkitARM (binutils only: `as`, `ld`,
`objcopy`, `nm`), and [agbcc](https://github.com/pret/agbcc) for C.

agbcc is a Linux program; on Windows it runs under WSL:

```sh
# inside WSL
git clone https://github.com/pret/agbcc ~/gba/agbcc
cd ~/gba/agbcc && ./build.sh      # the libgcc step may fail without ARM binutils; agbcc itself is enough
```

Override locations with `AGBCC` (dir containing `agbcc`), `WSL_DISTRO`, `DEVKITARM`.

Extract the clients from **unmodified** retail discs (SHA-1 checked against `config/versions.yml`):

```sh
python tools/extract.py GZLJ01 path/to/TWW-JP.iso
python tools/extract.py GZLE01 path/to/TWW-USA.iso     # extracts GZLE01 and GZLE01_ud
python tools/extract.py GZLP01_0 path/to/TWW-EUR.iso   # extracts all ten PAL clients
```

## Build

```sh
python tools/build.py all        # or one version
```

This disassembles `build/<ver>/prog.bin` into `build/<ver>/asm/`, compiles `src/*.c` for the
ranges in `config/<ver>/splits.txt`, links at `prog_base`, checks the program SHA-1, repacks it,
and checks the client SHA-1 against the retail file. Output: `build/<ver>/<client>.bin`.

Game bytes never enter git: `orig/` and `build/` are ignored, and the assembly is regenerated
from the extracted binary on every build.

## Decompiling a function

1. Find it in `build/<ver>/functions.txt` (`addr size mode name`) and its asm in
   `build/<ver>/asm/seg_*.s`.
2. Name it and its callees/globals in `config/GZLJ01/symbols.txt`
   (`Name = 0xADDR; // thumb|arm|data`), then `python tools/port_symbols.py GZLJ01 GZLE01 --write`
   (and `GZLE01_ud`) to carry the names over; check what it prints. A symbol only one US
   build has goes after the `// <ver> only` line at the end of that version's file; porting keeps it.
   A JP name listed there too is pinned: porting keeps the address written there. Some data
   ports to the wrong place when a US function's literal pool differs (see the "pinned" notes
   in the US files), so check new data symbols against the US asm. Addresses outside the image (IWRAM/EWRAM variables,
   I/O) become absolute symbols. Shared C uses these names so one source serves all versions;
   `VERSION_GZLJ01` etc. are defined for version differences.
3. Write the C in `src/`, add `src/file.c 0xSTART 0xEND` to each version's `splits.txt` (END =
   the next function's start, so the literal pool is included).
4. `python tools/build.py all`. On a mismatch it reports the first differing address and function.
   `python tools/asmdiff.py <ver> <function>` shows that function side by side (original vs
   built). It finds the built copy by symbol, so a size change earlier in the file doesn't
   misalign it. Branch and literal-pool offsets differ whenever sizes do, so look past those;
   `asmdiff.py -u` masks them and aligns the two listings with difflib, which is easier to
   read once an instruction is added or dropped.
5. To try variants quickly, `python tools/scratch.py <ver> <function> file.c` compiles one C
   file, links it with the function at its original address against the last build's
   symbols, and prints MATCH or the masked diff (about 3 s, against 20 s for `build.py all`).
   `--comment` takes the near-miss comment above `INCLUDE_ASM(<function>)` and writes it
   uncommented into a copy of its source file under `build/<ver>/scratch/`.
   `python tools/vary.py <ver> <function> file.c variants.py` scores a list of rewrites (each a
   list of `(old, new)` text replacements) in one go; see its docstring. When only registers
   differ, `python tools/prio.py <ver> <function> file.c` (or `--comment`) prints agbcc's
   global-alloc ranking (refs, live length, priority, hard register per pseudo; `--rtl N`
   shows where pseudo N is used), to see which two values' priorities need to swap.
6. For a near-miss, [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) tries
   random rewrites. It runs in WSL: `git clone https://github.com/simonlindholm/decomp-permuter
   ~/decomp-permuter` (or set `PERMUTER` to a clone), `pip3 install --user
   --break-system-packages toml pycparser`, and ARM binutils on PATH or unpacked without root
   (`apt-get download binutils-arm-none-eabi && dpkg -x binutils-arm-none-eabi_*.deb
   ~/arm-binutils`). `python tools/permuter/perm.py <ver> <function> --comment` (or a C file)
   writes `build/<ver>/permuter/<function>/` (base.c cut down to that function, target.o from
   the original asm, `tools/permuter/compile.sh`); add `--run -j 4 -- --better-only
   --stop-on-zero` to start it. Improvements land in `output-<score>-<n>/` there (`diff.txt`
   against base.c); check them with scratch.py.
7. When combine drops or keeps an extension for no visible reason, `tools/agbcc_dbg/` has a
   patch for an agbcc that traces combine's decisions (see its README).

Functions not decompiled yet stay in their C file as `INCLUDE_ASM(Name);` (from
`build/<ver>/asm/funcs/`, written for every function in a C range). A second line
`src/file.c .rodata 0xSTART 0xEND` in `splits.txt` places the file's `.rodata` in the shared
`.rodata` block (see `main.c`).

In C (no `INCLUDE_ASM` is left in any of these files): `src/main.c`,
`src/mode1.c`, `src/gba_com.c`, `src/joy_com.c`, `src/mode0.c`, `src/multi_sio.c`,
`src/heap.c`, `src/libc/rand.c`, and
the game code from `02018884` to the sound driver (all of it, in every version), in five
objects: `src/sprite.c`, `src/map.c`,
`src/message.c`, `src/item_menu.c` and `src/effect.c` (docs/layout.md says why they end where
they do), and `src/sound.c`. These
and main.c, mode0.c, mode1.c, gba_com.c, multi_sio.c and sound.c define their `.rodata` in C.
The ARM mixer is hand-written assembly (layout.md, "Sound driver").
Near-misses are kept as comments above their `INCLUDE_ASM`. Newlib's `bzero`
doesn't match with agbcc or old_agbcc yet: the SDK's newlib was built with other flags,
and old_agbcc at least matches its leaf-function prologue.

## Layout

```
config/versions.yml        target hashes and layout per version
config/<ver>/symbols.txt   names (and extra function seeds for the disassembler)
config/<ver>/splits.txt    address ranges built from C
include/  src/             headers, C sources, packer.s
tools/extract.py           disc -> orig/<ver>/, unpack -> build/<ver>/{prog,tail}.bin
tools/disasm.py            recursive-descent ARM/Thumb disassembler (capstone) -> GAS
tools/build.py             build + verify
tools/lz77.py              byte-matching GBA LZ77 codec
tools/port_symbols.py      copy GZLJ01's names to another version by matching functions
tools/progress.py          share of .text built from C, per version
tools/scratch.py, vary.py  compile one function against the last build; score variants
tools/prio.py              agbcc's register-allocation priorities for one function
tools/permuter/            decomp-permuter import script and agbcc compile script
tools/agbcc_dbg/           combine-tracing agbcc patch and runner
docs/                      layout.md (sections, modules), protocol.md (GC<->GBA link)
```

## Open questions

See [docs/layout.md](docs/layout.md) for the program's sections and modules and
[docs/protocol.md](docs/protocol.md) for the JOY bus protocol. The earlier questions about the
tail block, `client_ud.bin`, `msg_LZ.bin` and the packer's copy are answered in layout.md,
and so is the sound bank's format (see "Sound driver" and "Raw tail block" there).

- What `gMode1Unused_02023E38` is (12 unreferenced bytes; the readings ruled out are in
  layout.md).
- Whether the US message data renumbered its `F6` voice-clip codes; if not, the US lost an
  entry of `gMsgSoundIds` without adjusting `PlayMsgSound` (layout.md, sound IDs).
