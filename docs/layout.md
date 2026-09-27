# Program layout

What the unpacked program (`build/<ver>/prog.bin`) contains. Addresses are GZLJ01 unless a
column says otherwise. Names are in `config/<ver>/symbols.txt`: GZLJ01's file is the source
of truth, and `tools/port_symbols.py` copies names to the other versions.

## Sections

The image is a flat EWRAM multiboot program: `.text`, then `.rodata`, then the load image
of `.data`. crt0 does not copy anything; `InitSystem` clears IWRAM `0x03000000–0x03007B00`
and then `CpuSet`s the `.data` image to `0x03000000`.

| Section | GZLJ01 | GZLE01 | GZLE01_ud |
|---------|--------|--------|-----------|
| `.text` | `02014800–02023DEC` | `0201B000–0202AA48` | `0201B000–0202AC7C` |
| `.rodata` | `02023DEC–0202D290` | `0202AA48–02033928` | `0202AC7C–02033B5C` |
| `.data` (LMA) | `0202D290–0202E37D` | `02033928–02034A15` | `02033B5C–02034C49` |
| `.data` (VMA) | `03000000–030010ED` | same | same |
| sound bank (packer tail) | `0202F000–0203F844` | `02034E00–0203FEB4` | same as GZLE01 |

The end-of-`.text` and `.data` bounds come from `InitSystem`'s literal pool (the
`IntrTable` source is the first `.rodata` object, and the `.data` copy uses start/end
symbols). `.data` is `0x10ED` bytes in every version:

- `03000000` newlib `impure_data` (`struct _reent`, `0x2EC` bytes; `_rand_next = 1` at +0x58)
- `030002EC` `_impure_ptr`
- `030002F0–030010ED` newlib's `rand.o` DWARF (`.debug_*`, strings from
  `/tantor/build/nintendo/arm-000512/...`). The SDK linker script's `.data` rule evidently
  catches rand.o's debug sections, so they are copied to IWRAM at boot. Harmless and unused.

IWRAM layout (identical in all three versions):

| Address | Name | Notes |
|---------|------|-------|
| `03002F70` | `gIntrTable` | 14 handlers, copied from `gIntrTableInit` |
| `03003080` | `gMain` | +0x000: IWRAM copy of `intr_main` (0x400); +0x400: game code (`*(u32 *)0x020000AC`, from the packer's header); +0x416/0x418: held/new keys; +0x45C: heap (0x1800) |
| `03004CE0` | `gOamBuffer` | 0x400 bytes, reset to `{0xA0, 0}` pairs (y = 160, hidden) |
| `03005320` | `gGbaComDataStatus` | `u8[16]` per-slot link status |
| `03005390` | `gMultiSioArea` | 0x180, MultiSio work area (unused) |
| `03005510` | `gJoyCom` | 0x130, JOY bus work area, see [protocol.md](protocol.md) |
| `0300213C` | | IWRAM copy of the three ARM PCM routines |

EWRAM below the program holds the link receive buffers (`gGbaComBuffers`, one per slot):
`prog_base - 0xA800` (0xA000 bytes, slots 0 and 2: message data and map image) and small
buffers from `prog_base - 0x800`.

## `.text` modules

Link order is roughly: crt0, game code, MultiSio, JoyCom, more game code, sound driver, ARM
mixer, libagbsyscall, libgcc, newlib. Object boundaries marked "?" are guesses; the
`.rodata` order (one block per object, in link order) is the best evidence for the rest.

| GZLJ01 | GZLE01 | Module | Contents |
|--------|--------|--------|----------|
| `02014800` | `0201B000` | `crt0.s` | stock AGB SDK: `_start`, header, `start_vector`, `intr_main` |
| `02014A0C` | `0201B20C` | `main.c` | `AgbMain`, `VBlankIntr`, `IntrDummy`, `ReadKeys`, `InitSystem`, `MainLoop`, ... |
| `02015xxx` | | game code | not yet mapped |
| `02017190` | `0201D980` | `gba_com.c`? | app-side link API over JoyCom (status per slot, send/recv) |
| `020173B8` | `0201DBA8` | `multi_sio.c` | Nintendo MultiSio (`"MultiSioSync010316"` is its `.rodata`); nothing calls it |
| `02017824` | `0201E014` | `joy_com.c`? | JOY bus link: init/stop, IRQ handler (copied to IWRAM), send queue |
| `02018658` | `0201EE3C` | `heap.c`? | `HeapInit`/`HeapAlloc`/`HeapFree` (the JOY IRQ copy lives in this heap) |
| `02018xxx` | | game code | not yet mapped |
| `02020E28` | `02027A84` | sound | `SoundInit` -> `SoundDriverInit(gSoundBankHeader)`; custom PCM driver, not m4a |
| `020237CC` | `0202A428` | ARM mixer | `PcmClipS16ToS8`, `PcmMixChannel`, `PcmEchoMix` (run from IWRAM) |
| `02023A84` | `0202A6E0` | libagbsyscall | `ArcTan2 CpuFastSet CpuSet Div LZ77UnCompVram LZ77UnCompWram Sqrt`, then `JoyHardReset` |
| `02023AC0` | `0202A71C` | libgcc | `_call_via_*`, `__divsi3 __div0 __modsi3 __udivsi3 __umodsi3` |
| `02023D9C` | `0202A9F8` | newlib | `bzero`, `srand`, `rand` |

Interrupts: `gIntrTableInit` puts `JoyComIntr` in the serial slot and `VBlankIntr` in
VBlank. The rest are `IntrDummy`. `JoyComInit` then points the serial slot at the IWRAM copy.

## Version differences

- **GZLJ01 vs GZLE01**: same modules in the same order. Changes are mostly in game code
  (text/UI; the matcher pairs 337 vs 339 functions, with a few added or reshaped functions).
  Libraries are identical.
- **GZLE01 vs GZLE01_ud** (`client_ud.bin`): byte-identical except one function,
  `sub_02024AB8` (JP `sub_0201DEAC`), which grows by `0x234` bytes. Everything after it
  shifts by `0x234`, and `.rodata`/`.data` are the same apart from relocated pointers. The
  extra code reads L (bit 9 of the new-keys word at `gMain+0x418`) to toggle a flag, and uses
  the D-pad to step a counter with extra divisions. It looks like a debug or tuning mode,
  consistent with "ud" = "USA debug". The game only loads `client_u.bin`.

## Answers to earlier open questions

- **Raw tail block**: it's the sound bank. `gSoundBankHeader` (`.rodata`, GZLJ01
  `0202CD20`) holds seven pointers into it: start, end, and offsets `+0x10`, `+0x674`,
  `+0x93C`, `+0x944`, `+0x94C`. `SoundInit` passes it to the driver. `+0x94C` starts with a
  count (8) and an offset table, followed by most of the block (`0xFEF8` bytes). That data
  behaves like signed 8-bit PCM (mean |Δ| ≈ 10, well under mean |x| ≈ 25), which is what
  `PcmMixChannel` reads (`ldrsb`, 8.8 fixed-point position). The smaller tables are
  probably sequence/instrument data; not decoded yet.
- **Packer "CpuSet copies half"**: it doesn't. The stub's ARM `svc #0x0C0000` is BIOS
  function `0x0C`, **CpuFastSet**, which always moves 32-bit words, so a word count is
  right. CpuFastSet rounds the count up to 8 words, so it reads up to 28 bytes past the end
  of the file and writes them past the tail. That's harmless.
- **`msg_LZ.bin` round-trip**: it's one plain stream, packed **VRAM-safe** (no distance-1
  matches) and zero-padded to 4 bytes. `lz77.compress(raw, min_dist=2)` plus padding
  reproduces both the JP and US files byte for byte.
