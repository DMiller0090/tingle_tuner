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

## What the binary is

Each client is a GBA multiboot image with two layers:

1. **Packer** (`src/packer.s`, identical code in every version): standard header, a JOY bus
   handshake at `0xE0` (sends the game code, waits for the GC to echo it), then `CpuSet` copies a
   raw tail block up to `tail_dest`, `LZ77UnCompWram` unpacks the program to `prog_base`, and it
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
2. Name it and its callees/globals in `config/<ver>/symbols.txt` for **each** version
   (`Name = 0xADDR; // thumb|arm|data`). Addresses outside the image (IWRAM/EWRAM variables,
   I/O) become absolute symbols. Shared C uses these names so one source serves all versions;
   `VERSION_GZLJ01` etc. are defined for version differences.
3. Write the C in `src/`, add `src/file.c 0xSTART 0xEND` to each version's `splits.txt` (END =
   the next function's start, so the literal pool is included).
4. `python tools/build.py all`. On a mismatch it reports the first differing address and function.

`src/main.c` (`AgbMain`) is the worked example.

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
```

## Open questions

- What the raw tail block is (`0x10844` bytes JP / `0xB0B4` US). It looks like PCM sample
  data for the ARM mixer.
- What distinguishes `client_ud.bin` from `client_u.bin` (same layout; `+0x234` bytes of program).
- `msg_LZ.bin` (the tuner's text, sent separately via `mDoGac_SendDataSet`) doesn't round-trip
  through `lz77.compress` yet. It likely isn't one plain stream.
- The packer's `CpuSet` uses 16-bit mode with a word count, so it copies only half the tail.
  Either that's harmless or the second half is unused; worth checking on hardware or in an emulator.
