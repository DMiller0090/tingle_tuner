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
| `03003080` | `gMain` | +0x000: IWRAM copy of `intr_main` (0x400); +0x400: game code (`*(u32 *)0x020000AC`, from the packer's header); +0x414: VBlank flag; +0x416/0x418: held/new keys; +0x41A-0x447: shadow display registers (DISPCNT ... BLDY, copied by `VBlankIntr`); +0x44C-0x459: frame/mode/link bytes (`mode` indexes `gMainModeFuncs`, `modeState` the mode's state table); +0x45C: heap (0x1800) |
| `03004CE0` | `gOamBuffer` | 0x400 bytes, reset to `{0xA0, 0}` pairs (y = 160, hidden) |
| `030050E0` | `gGameState` | 0x240, game state; starts with the GC's `daAgb_Flags` (slot 9, see `include/gba_com.h`) |
| `03005320` | `gGbaComDataStatus` | `u8[16]` per-slot link status |
| `03005390` | `gMultiSioArea` | 0x180, MultiSio work area (unused) |
| `03005510` | `gJoyCom` | 0x130, JOY bus work area, see [protocol.md](protocol.md) |
| `030010F0` | `gSoundPlayerSoundId` ... `gSoundPlayers` | sound driver work area (to `03002F64`), see [Sound driver](#sound-driver) |
| `0300213C` | `gPcmCode` | IWRAM copy of the three ARM PCM routines |

EWRAM below the program holds the link receive buffers (`gGbaComBuffers`, one per slot,
named `gGbaComBuf*` in symbols.txt): `prog_base - 0xA800` (`gGbaComMessageBuf`, 0xA000
bytes, slots 0 and 2: message data and map image) and small buffers from `prog_base - 0x800`.
Right after them, at `prog_base - 0x4C0`, are `gBgScrollTables` (`u16[160]` per table), which
`VBlankIntr` feeds to BG0HOFS by HBlank DMA while `gMain.hblankScroll` is set.
They keep their offset from `prog_base` in every version, and `port_symbols.py` relies on that.

## `.text` modules

Link order is roughly: crt0, main, mode 1 game code, GbaCom, MultiSio, JoyCom, mode 0, heap,
more game code, sound driver, ARM mixer, libagbsyscall, libgcc, newlib. Object boundaries marked "?" are guesses; the
`.rodata` order (one block per object, in link order) is the best evidence for the rest.

| GZLJ01 | GZLE01 | Module | Contents |
|--------|--------|--------|----------|
| `02014800` | `0201B000` | `crt0.s` | stock AGB SDK: `_start`, header, `start_vector`, `intr_main` |
| `02014A0C` | `0201B20C` | `main.c` | `AgbMain` ... `MainLoop` (ends `02015054`); `.rodata` `02023DEC–02023E38` |
| `02015054` | `0201B854` | `mode1.c` | `InitDisplayIntr`, `SeedRandom`, `UpdateRandom`, the HUD/graphics helpers, `Mode1State0-5`, `Mode1RecvMarkers` (slot 3), `MainMode1` (ends `02017190`); `.rodata` `02023E38-02023E90`: an unreferenced 12-byte block, `gDungeonMsgIds`, `gMode1StateFuncs`, two u16 tables. The tables on both sides of `gMode1StateFuncs` put them in one object with `MainMode1`. The random/display functions have no `.rodata` or other marker, so whether they are a separate object can't be told from the binary; they stay in mode1.c |
| `02017190` | `0201D980` | `gba_com.c` | app-side link API over JoyCom (status per slot, send/recv) |
| `020173B8` | `0201DBA8` | `multi_sio.c` | Nintendo MultiSio (`"MultiSioSync010316"` is its `.rodata`); nothing calls it |
| `02017824` | `0201E014` | `joy_com.c` | JOY bus link: init/stop, IRQ handler (copied to IWRAM), send queue (ends `02018158`) |
| `02018158` | `0201E948` | `mode0.c` | title screen while the GC sends the boot data: `Mode0State0-3`, `Mode0DrawTitleSprite`, `Mode0UpdateBlend`, `MainMode0`; `.rodata` `02023EE4-02023EFC` (US: `0202AB40-0202AB64`, which adds `gMode0UsString` for the US-only `DrawString`) |
| `02018658` | `0201EE3C` | `heap.c` | `HeapInit`/`HeapAlloc`/`HeapFree`/`HeapCompact` (the JOY IRQ copy lives in this heap) |
| `02018884` | `0201F068` | `sprite.c` | easing/step helpers, `DrawSprite`/`DrawSpriteAt`/`DrawSpriteParts`; `.rodata` `gSinTable` (`02023EFC–0202417C`, 320 × s16 8.8 sine, so `cos(i) = gSinTable[i + 64]`) and `gSpriteTable` |
| `0201920C` | `0201F9F0` | `map.c` | the map screen, map icons, quest markers, `UpdateSelection`-`ExamineSelection`; `.rodata` `020241D4–0202429C` |
| `0201BA2C` | `02022228` | `message.c` | text renderer, choice menu, messages (`MsgStep`, `ShowMessage`, US `DrawString`); `.rodata` `0202429C–02024460` |
| `0201DA78` | `02024684` | `item_menu.c` | item menu, `UseItem`, HUD panels; `.rodata` `02024460–020244D0` |
| `0201EBAC` | `020257B8` | `effect.c` | item effects, `MainMode2`, objects, wave scroll, Select window, status icon; `.rodata` `020244D0–02024568` |
| `02020E28` | `02027A84` | sound | custom driver, not m4a: API (`SoundInit` ... `SoundPlayLoop`), core from `SoundDriverInit` (`020213C0`), command wrappers `0202328C–02023510`; `.rodata` `gSoundBankHeader` ... `gVibratoTable` (`0202CD20–0202D28C`, in C in `src/sound.c`). See [Sound driver](#sound-driver) |
| `020237CC` | `0202A428` | ARM mixer | `PcmClipS16ToS8`, `PcmMixChannel`, `PcmEchoMix` (hand-written; run from IWRAM) |
| `02023A84` | `0202A6E0` | libagbsyscall | `ArcTan2 CpuFastSet CpuSet Div LZ77UnCompVram LZ77UnCompWram Sqrt`, then `JoyHardReset` |
| `02023AC0` | `0202A71C` | libgcc | `_call_via_*`, `__divsi3 __div0 __modsi3 __udivsi3 __umodsi3` |
| `02023D9C` | `0202A9F8` | newlib | `bzero`, `srand`, `rand`; the last 4 bytes of `.rodata` (`0202D28C`, `"C"`) are `impure.o`'s locale string, which `impure_data` + 0x34 (`_current_locale`) points at |

### Game code `02018884–02020E28`: what the `.rodata` says

Each object's `.rodata` is one block, in link order, so a table referenced by only one
function belongs to that function's object. Two such tables in the wrong order therefore put
their functions (and everything between them) in one object. Function-pointer tables tie
their targets to the object too. And a block starts at its object's largest alignment (4 once
it holds a pointer table), in every version. Nothing in the binary marks where an object
ends, so this only ever puts functions together:

| Functions | Evidence |
|-----------|----------|
| `UpdateMap` (`0201A224`) ... `Mode1VBlank` (`0201ABDC`) | `Mode1VBlank` uses `gSeaAnimFrames` (`02024235`), before `UpdateMap`'s `gSeaAnimPeriods` (`0202423A`) |
| `MapIconFollowMarker` (`0201AC94`) ... `UpdateMapIcons` (`0201AF48`) | pointer table `gMapIconFuncs` (`02024244`), used by `UpdateMapIcons` |
| `ChoiceMenuUpdate` (`0201C204`) ... `MsgStep` (`0201CB10`) | `MsgStep`'s pointer table `gMsgCmdFuncs` makes its block 4-aligned. In the US builds `gMsgTingleAnims` (`0202AFBA`) and `gMsgSoundIds` (`0202AF76`) are not, so the block starts at `gTwoChoiceIds` (`ChoiceMenuUpdate`) or earlier |
| `DrawGlyphRun` (`0201C660`) ... `MsgStep` (`0201CB10`) | `MsgStep` uses `gMsgTingleAnims` (`02024354`), before `DrawGlyphRun`'s `gGlyphRunStart` (`02024376`); `gMsgCmdFuncs` (`020243A4`) lists the 19 handlers `MsgCmd00-12` |
| `ItemMenuUpdate` (`0201DEAC`) ... `ItemMenuInit` (`0201E190`) | `ItemMenuUpdate` uses `gChestRowIds` (`020244C0`), after `ItemMenuInit`'s `gShopItemsInit` (`02024460`) |
| `EffectBombBlast` ... `ObjBurst` (`0202081C`) | `gItemEffectFuncs` (`020244E4`, 18 handlers `0201EC10-02020190`, run by `UpdateItemEffect`), `gMode2StateFuncs` (`0202452C`, `Mode2State0-1`), `gObjFuncs` (`02024534`, entry 0 `ObjBurst`) and `gObjBurstStateFuncs` (`02024560`) |

The five source files keep these groups whole. Between two groups the boundary is a choice
within the range the tables allow, made by who calls the functions in between:

- `sprite.c` / `map.c` at `CopyMapColumn`: `DrawSpriteAt`/`DrawSpriteParts` are called from
  everywhere, `CopyMapColumn`/`CopyMapRow` only by the map code (`ScrollMap`,
  `InitMapView`). The easing helpers have no `.rodata` and nothing separates them from
  `DrawSprite`, so they share `sprite.c`.
- `map.c` / `message.c` at `DrawMsgBoxRow`: the text renderer's functions are only called
  from the message code; `ExamineSelection` calls `ShowSelectionMsg` (whose `gFriendMsgIds`
  ends the map block) and both are cursor messages from `Mode1State2`.
- `message.c` / `item_menu.c` at `DrawSlotCursor`: the item menu's first table is
  `gShopItemsInit`; `MsgReset` ... `MsgClose` are message functions.
- `item_menu.c` / `effect.c` at `EndItemEffect`: `EndItemEffect`/`StartBombBlast` are called
  by the effects (and `MainMode1`), `UpdateHudPanels` (`gHudPanelTiles`) by the item menu.
- `effect.c` runs to the sound driver: `BuildWaveScrollTable` ... `DrawStatusIcon` have no
  `.rodata`, so nothing separates them from the objects before them.

What the range does, in address order (names in symbols.txt):

- `02018884-020190A8`: easing/step helpers (`EaseToward*`, `StepToward*`, `CountDown*`),
  `DrawSprite`/`DrawSpriteAt` (sprites are `struct SpriteFrame` lists of OAM parts;
  `gMain.oamCount`/`affineCount` count the OAM entries and affine matrices used this frame).
- `020190A8-0201AFCC`: the map screen. BG3 map streaming (`CopyMapColumn/Row`,
  `ScrollMap`), `DrawMapMarkers`, `HighlightMapRooms`, the view following a marker, the
  sea chart (`UpdateMap`, the 7x7 squares and their tile animation), the floor number, the
  map icons (`gGameState.icons`: marker follower, target cursor, blinking target), the target
  compass (`UpdateTargetCompass`), and `DrawQuestMarkers` (pearls, Triangle Islands, temples
  from `GbaComFlags` byte 9).
- `0201AFCC-0201C660`: the text renderer (a font at `0x0600A000` drawn onto a 12-tile
  canvas at `0x0600F100`, optionally scaled 2x through `TEXT_GLYPH_BUF`), the text box frame
  and the 2-/3-way choice menu (`ChoiceMenuUpdate`).
- `0201C660-0201DA78`: messages. `MsgStart` looks the text up through the offset table of
  the message data (`msg_LZ.bin`, unpacked to `0x02000000`), `MsgStep` runs one character
  or control code (`F2`-`FF`) per call, and `ShowMessage` (the old `GamePlaySound`) starts
  one with Tingle's voice and animation. The game's "sound IDs" in `ShowMessage` calls are
  message IDs.
- `0201DA78-0201F308`: the item menu (12 items, `struct ItemSlot`: icon, OBJ tile, price
  in rupees; a 5x3 grid, row 2 the dungeon chests), `UseItem` (slot 6), the HUD panels and
  the timer.
- `0201F308-02020E28`: the item effects (`effect[0]` from slot 7; `Effect7` is the shop,
  slots 12/13), the object system (`gObjs`, `ObjSpawn`, `ObjBurst`), `MainMode2` (the link-lost
  screen: two states), the per-line wave scroll, the select-button window wipe and the status icon.

### Markers, selection and items

The map shows the markers of slot 3 (`struct MapMarker`, built by the GC's `dMap_c`, see
[protocol.md](protocol.md)): marker 0 is Link (type 1), marker 1 the Tingle Tuner cursor
(type 3), then enemies (2), the ship (4), doors (17), friends (19) and others. A player or
friend marker's `param` is the character (0: Link; `gCharIcon0-4`, also the status icon), and
`UseItem` refuses items 3-5, 8-10 and 16 while Link's is not 0. `UpdateSelection` (every
frame in state 2) looks for a marker within 8 px of the cursor, or on the sea for the spot of
the cursor's square (slot 14); A (`ExamineSelection`) sends a pending switch record, or has
Tingle think (messages `15`-`17`, `reactionTimer`) before `ShowSelectionMsg` shows the
marker's message (`gbaName`) or the square's name; markers of types 5, 12, 13 and 16 are
drawn hidden (`gSpriteTable[7]`) until examined (`revealedType*`). When Link's marker enters
another sea chart square, the square is named 3-8 s later (`squareMsgPending`).

Items (`gGameState.items`, from `gShopItemsInit`; B uses `selectedItem`, A with nothing
selected uses 16). Most go to the GC in slot 6 and the answer in slot 7 (`effect[0]`) starts an
item effect (`gItemEffectFuncs`); the GC side is `daAgb_c::GbaItemUse`. The in-game names are
presumably the Tingle Tuner shop's:

| item | price (discount) | GBA -> GC | effect |
|------|------------------|-----------|--------|
| 1 | 0 | - | puts the target (`icons[2]`) on the cursor; sea only |
| 2 | 10 | 2 | bomb: GC answer 2 (15 with a rupee drop when following a target) blasts at the cursor (`EffectBombBlast`); answer 14 when the cursor sits on Link starts a countdown (`Effect14`) |
| 3 | 30 (20) | 0x11, then 3 | balloon: GC hover boots for 150 frames (`StartBalloonEffect`, effect 3) |
| 4 | 40 (30) | 0x12, then 4 | shield: GC Tingle shield for 300 frames (`StartShieldEffect`, effect 4) |
| 5 | 40 (30) | 5 | random: the GC picks 10 (full heal), 12 (balloon and shield), 13 (long shield) or 0x13 (nothing) (`Effect5`) |
| 6 | 0 | - | message `0D` only |
| 7 | 0 | 7, then slots 12/13 | the shop (`Effect7`): arrows, bombs or bait for rupees |
| 8-10 | 20/40/80 (10/20/40) | 0x14, then 8-10 | Tings: 3 hearts, magic, both full (`Effect8`: the title sprite brings the Ting down to Link) |
| 11 | 0 | - | in slot 6 until the shop is unlocked (message code `F3 05`): a choice dialogue (`Effect11`) |
| 16 | - | 16 | the GC's `MODE_LOOK_ATTENTION` (Link looks at the cursor) |
| 0x15 | - | 0x15 | message code `F3 12`: a bomb on Link (answer 21) |

The discounts come from the GC event flags `UNLOCK_TINGLE_BALLOON_DISCOUNT` (3-5) and
`UNLOCK_TING_DISCOUNT` (8-10), which the GBA sets itself through `GbaFlg.bits` when message
code `F3 00`/`F3 01` runs (`MsgCmd00`/`01`); the shop is event `0x1708`, set the same way by
`F3 05`.

`DrawSprite` (`02018AB8`) is the first function in the range that uses `.rodata` (it reads
`gSinTable + 0x80`, the cosine), so `gSinTable` is either its object's or a data object of
its own. `gSpriteTable` (`0202417C`, `struct SpriteFrame` lists) comes next. Mode 2's states
also use `gMode0Work`, so the link-lost screen reuses the title's blend.

`.rodata` `02024568–0202CD20` is large data (graphics and tables, not yet split) referenced from
all the game-code ranges, so it is probably separate data objects linked after the code's
own `.rodata`. `gSoundBankHeader` (`0202CD20`) follows it.

Interrupts: `gIntrTableInit` puts `JoyComIntr` in the serial slot and `VBlankIntr` in
VBlank. The rest are `IntrDummy`. `JoyComInit` then points the serial slot at the IWRAM copy.

### Sound driver

A small sequenced driver: 20 players run byte-code sequences on 24 tracks, whose notes go to
7 software PCM voices (mixed in stereo into DirectSound FIFO A = left, B = right, 10512 Hz,
176 samples per frame, double-buffered, DMA1/DMA2 restarted every VBlank by `PcmVSync`) or
to the four GB channels. The bank holds only sound effects: the song path exists but its
tables are empty (below).

**API** (main loop). `SoundInit` = `SoundDriverInit(&gSoundBankHeader)` + `SoundApiInit`.
`SoundPlay(id, params)` looks up `gSoundTable[id]` (44 entries JP, 43 US): bits 0-15 SE
index, 16-23 player (0-4 in use; 4 = the voice clips, IDs 0-6), 24 pan and volume from
`params->x` (`SoundPanFromX`/`SoundVolumeFromX`), 29 volume from `params->y`, 27 add the
u16 at `params+4` to the SE index, 28 don't restart the same ID, 25/26 low/high priority (a
sound only replaces one of equal or lower priority). It posts `SoundCmdStartSe(player, 0,
index)` plus pan/volume commands and records the ID in `gSoundPlayerSoundId[player]`.
`SoundPlayLoop(owner, id, params)` (was `Func_02021284`) plays `gSoundLoopTable[id]` (same
format, 11 words up to `gInstTypeToVoicePool`; the game uses IDs 0-9) on player 16
or 17 (`gSoundLoopSlots`); the caller repeats it every frame, and `SoundApiUpdate` (from
`SoundMain`) stops a slot the first frame it isn't refreshed. Loop 9 is special: `params+4` (s16) sets a tempo offset (`-30 * p`), and it
plays SE index `entry + 1` when restarted within 150 frames of stopping. `SoundPauseAll`,
`SoundStopAll`, `SoundStopMost`, `SoundIsPlaying` are unused.

**Sound IDs** (every `SoundPlay`/`SoundPlayLoop` call in the three builds, found by scanning
the disassembly for `bl`; the US calls are the JP ones with `SOUND_ID_SHIFT` applied). "SE"
is the sequence index in the SE set, "msg" the `F6 xx` message codes (`PlayMsgSound`, via
`gMsgSoundIds`) that play the ID in JP. Voice clips (player 4) are Tingle's sampled
voice. Tables passed to `ShowMessage` (`gDungeonMsgIds`, `gChestMsgIds`, `gFriendMsgIds`,
the choice tables, `switchMsg`) hold message numbers, not sound IDs.

| JP | US | SE | pl. | what plays it |
|----|----|----|-----|---------------|
| `00` | `00` | 2 | 4 | voice: the map screen comes in (`Mode1State1`, with the dungeon's message); msg `0A`, `0B` (entry 0) |
| `01` | `01` | 3 | 4 | voice: the cursor's marker reaches something selectable (`UpdateSelection`); msg `17` |
| `02` | `02` | 4 | 4 | voice: A on a selection shows its message (`ExamineSelection` result 2) |
| `03`-`05` | same | 5, 8, 6 | 4 | voice: messages only (msg `15`, `18`, `1A`; `16` is skipped) |
| `06` | - | 7 | 4 | voice, JP only: msg `1B` |
| `07` | `06` | 20 | 0 | `flags.isFree` goes off (`Mode1State2`); L released after panning the map (`UpdateMap`) |
| `08` | `07` | 23 | 0 | msg `13` |
| `09` | `08` | 24 | 0 | rupee counter reaches the count (`SE_RUPEE_COUNT_END`); msg `14` |
| `0A` | `09` | 26 | 0 | A on the map's target cursor places the target (`MapIconCursor`) |
| `0B` | `0A` | 25 | 0 | the target icon is put on the cursor's marker (`Mode1State2` after choice message `1A7`, `UseItem`) |
| `0C` | `0B` | 27 | 0 | tick: `flags.timer` changes (`DrawTimerCount`), effect 14's countdown |
| `0D`-`0F` | `0C`-`0E` | 28-30 | 0 | unused |
| `10` | `0F` | 31 | 0 | msg `12` |
| `11` | `10` | 21 | 0 | high priority; msg `11` |
| `12` | `11` | 22 | 0 | high priority: the chest counter starts blinking (`ChestCounterMain`); msg `10` |
| `13` | `12` | 34-36 | 0 | a message opens (`ShowMessage`, `MsgNewPage`): low priority, SE + random 0-2; msg `0C`-`0F` |
| `14` | `13` | 42 | 0 | item menu: an item is picked and flies to the slot |
| `15` | `14` | 43 | 0 | item menu: cursor auto-repeat, and the picked icon arriving |
| `16` | `15` | 32 | 0 | title screen appears (`Mode0State0`); `flags.isFree` goes on; L pressed to pan the map |
| `17` | `16` | 33 | 0 | message closed (`MsgClose`) |
| `18`, `19` | `17`, `18` | 10, 13 | 0 | unused |
| `1A` | `19` | 52 | 0 | choice menu pick; L/B open or close the sea chart overlay, A in it; with message `0F` in `UseItem` and for the GC's item answer 1; msg `01` |
| `1B` | `1A` | 53 | 0 | msg `02` |
| `1C` | `1B` | 54 | 0 | `SE_KEY_BLOCKED`: a key while the screen waits (`Mode1State4`), A while busy, the GC refusing an item (answer 0), picking the item already selected; msg `06` |
| `1D`, `1E` | `1C`, `1D` | 55, 56 | 0 | `SE_RUPEE_COUNT_UP` / `_DOWN`, every other rupee; msg `08`, `07` |
| `1F` | `1E` | 48 | 0 | `SE_LINKED`: all boot data received (`Mode0State3`); msg `00` |
| `20` | `1F` | 41 | 1 | msg `1E` |
| `21` | `20` | 40 | 1 | a switch record is sent to the GC (`Mode1SendSwitch` from `Mode1State2`, `ExamineSelection`); msg `1C` |
| `22` | `21` | 39 | 1 | msg `1D` |
| `23` | `22` | 47 | 1 | the GC's item effect 2/15/21 (the `ObjBurst` pieces) or 16 (`MainMode1`); msg `09` |
| `24` | `23` | 17 | 1 | msg `19` |
| `25` | `24` | 14 | 1 | chest counter blink, every 32 frames |
| `26` | `25` | 9 | 2 | msg `1F` |
| `27` | `26` | 46 | 2 | Select window opens or closes (`UpdateSelectWindow`) |
| `28` | `27` | 11 | 2 | low priority: text blip per character (`DrawChar` when no message loop runs, the player name in `MsgStep`) |
| `29` | `28` | 51 | 2 | cursor move: choice menu Up/Down, item menu first press; msg `05` |
| `2A` | `29` | 50 | 3 | item menu closed (START), chest counter done; msg `04` |
| `2B` | `2A` | 49 | 3 | item menu opened (START); msg `03` |

Loop IDs (`SoundPlayLoop`, same in all builds; SE index US = JP - 1):

| ID | SE | what keeps it playing |
|----|----|-----------------------|
| 0 | 12 | `MsgStep` while `charLoopSound` is set and no message loop runs |
| 1 | 15 | item effects 8-10 while the title sprite waits over Link's marker; msg `21` |
| 2 | 16 | the link-lost screen (`Mode2State1`); msg `20` |
| 3 | 18 | status icon 2 (`DrawStatusIcon`, `enemyHighlighted`) |
| 4 | 19 | status icon while `flags.followTarget` |
| 5 | 37 | title screen while the boot data comes in (`Mode0State1-3`); msg `22` |
| 6 | 38 | `Mode1State2` while `reactionTimer` runs (the random reaction of `ExamineSelection` result 1) |
| 7 | 44 | `Mode1State2`: `flags.moveSpeed` set and `flags.isFree` |
| 8 | 45 | `UpdateMap` while the D-pad pans the map (L held) |
| 9 | 0 | `Mode1State2`: `flags.moveSpeed` set, not free, not following; tempo from `(moveSpeed - 1) ^ 3` |
| 10 | 0 | unused |

The message codes `F6 20`-`F6 22` pick loop IDs (`gMsgSoundIds[0x20-0x22]`). The US table
lost entry 27 (voice clip 6), but `PlayMsgSound`'s ranges (`0C-0F` random, `20-22` loop, `16`
skipped) are unchanged, so in the US every code from `1B` reads the next entry, and `F6 22`
reads past the table into `gMsgTingleAnims[0]` (2). Unless the US message data renumbered its
codes, that is a US bug (the message data isn't extracted, so this isn't checked).

**Command queue.** API calls never touch the players: each wrapper writes a 12-byte
`{u16 cmd; u32 a; u32 b}` into `gSoundCmdQueue` (46 entries, `SoundCmdPush`),
`SoundMain`'s `SoundCmdFlush` publishes them, and `SoundVSyncProc`'s `SoundCmdProcess`
runs them through `gSoundCmdHandlers[cmd >> 8]`:

| cmd | wrapper | effect |
|-----|---------|--------|
| `000` | `SoundCmdStartSong(player, song)` | `PlayerStartSong` (unused) |
| `001` | `SoundCmdStartSe(player, set, index)` | `PlayerStartSe`: one track at `se[set] + offsets[index]` |
| `002` | `SoundCmdFadeOut(player, frames)` | `PlayerFadeOut` (0 = stop) |
| `003` | `SoundCmdPause(player, on)` | `PlayerSetPause` |
| `004` | `SoundCmdSetTempo(player, s16)` | tempo offset (player +0x32) |
| `005` | `SoundCmdSetVolume(player, u8)` | player +0x41 |
| `006` | `SoundCmdSetFlags(player, u8)` | player +0x44; bit 0 sends notes to `gSoundNoteHook` |
| `100–103` | `SoundCmdMuteTracks`, `...SetTrackPan`, `...SetTrackVolume`, `...SetTrackBend(player, mask, range, bend)` | per track in `mask` |
| `200–201` | `SoundCmdFadeOutPlayers(mask, frames)`, `SoundCmdPausePlayers(mask, on)` | per player in `mask` |
| `300–304` | `SoundCmdNop300`, `SoundCmdCall(func, arg)`, `SoundCmdSetSeqHook`, `SoundCmdSetNoteHook`, `SoundCmdSetPcmVoices(n)` | global |

The game uses only `001`, `002`, `004`, `101` and `102` (`200`/`201` only from the unused
stop/pause functions).

**Core** (VBlank). `SoundVSync` = `PcmVSync`. `SoundVSyncProc`: `SoundCmdProcess`,
`PlayerUpdateAll` (fades; `TrackExecute` on each track: wait counter += length × 150,
minus tempo (default 150) per frame, so one tick is one frame), `CgbUpdate`, then (if
`gPcmEnabled`) `PcmMixFrame` (clears `gPcmMixBuffer`, `PcmMixVoice` per active voice via `gPcmMixFunc` =
`PcmMixChannel`, then `gPcmClipFunc` = `PcmClipS16ToS8` into the DMA buffer).
`TrackNoteOn` resolves the instrument (`TrackLookupInstrument`), allocates a voice
(`VoiceAlloc`: PCM from `gPcmFreeVoices` or by stealing the lowest priority, or the
instrument's GB channel) and starts it. Per voice per frame: `EnvelopeStep`,
`VoiceUpdatePitch` (sweep, bend, vibrato from `gVibratoTable`), `PcmVoiceVolume` /
`CgbVoiceVolume`. `gPcmEchoFunc` (`PcmEchoMix`: adds an echo buffer into the mix, then scales the echo down by
2^shift) is set up but never called. Sequence command `E3` sets track +0x4C; `TrackNoteOn`
and `PcmMixFrame` copy it to voice +0x1A, and nothing reads that (not even through
`gSoundNoteHook`, which the game never installs), so `E3` has no audible effect. It was
plausibly an echo send for `PcmEchoMix`.

The ARM mixer (`PcmClipS16ToS8`, `PcmMixChannel`, `PcmEchoMix`, JP `020237CC-02023A84`) is
hand-written assembly, copied to IWRAM as one block (`CpuFastSet(PcmClipS16ToS8, gPcmCode,
0xD8)`). pret's `agbcc_arm` (the same GCC 2.9) can't produce it: signed-division rounding is
`cmp rX, #0; addmi` (GCC emits `addlt`); `PcmMixChannel` loads its four stack arguments with
`mov ip, sp; push {r4-r8}; ldm ip, {r4-r7}`; constants live in r4/r5 and lr is never used
(GCC hands out ip and lr first); the clip test is `ands; cmpne; addne` with no boolean; every
unrolled step is the same `ldrsh [rN]` / `strh [rN], #2`; and `PcmMixChannel` has two loops
(4x unrolled, and the remainder) that each return on their own.

Work area structs (sizes from the loops; offsets from the code):

- player (`gSoundPlayers`, 20 × 0x48): +0 bank map, +4 SE offset table, +8 track pointers
  [10], +0x30 tempo, +0x32 tempo offset, +0x34 fade volume (0x8000), +0x36/+0x38/+0x3A fade
  step/target/frames, +0x3C bit 0 paused, +0x40/+0x41 volume, +0x42 state (0 idle, 1
  playing, 2 fading), +0x43 SE player, +0x44 flags.
- track (`gSoundTracks`, 24 × 0x54): +0 pc, +4 sample set, +8 player, +0xC voice list,
  +0x10/+0x14/+0x18 vibrato delay/speed/depth, +0x1C-0x23 pitch sweep, +0x24 call stack,
  +0x30 stack pointer, +0x34 wait, +0x40 bank, +0x42 program, +0x44/+0x46 last length/wait,
  +0x48 velocity, +0x49 tie, +0x4A mute, +0x4B pan, +0x4C (E3, see below), +0x4D/+0x4E volume, +0x4F
  bend, +0x50 bend range, +0x51 transpose, +0x52 priority, +0x53 notes wait.
- voice (`gPcmVoices` 7 × 0x78, `gCgbVoices` 4 × 0x78): +0 type (0 PCM, 1-4 GB channel), +1
  state (0 free, 1 on, 2 released), +4 track, +8 priority, +9 key, +0xA velocity, +0xC
  frequency, +0x14 volume, +0x18 gate, +0x1C pan, +0x20-0x3C vibrato and sweep, +0x40-0x50
  envelope, +0x54 instrument, +0x58 release, +0x5C sample, +0x60 position (GB: frames since
  the note started), +0x64 GB data (wave pointer; duty list pointer when instrument flags bit 0
  is set, a u16 count then the duties; else the duty byte), +0x68/+0x6C list links, +0x70/+0x74
  track's voice list.

GB voices (`TrackNoteOn`, `CgbVoiceStart`, `CgbUpdate`): key splits and drum kits play at key
0x30. With instrument flags bit 4 set the note length is divided by the tempo alone (no tempo
offset). A pitch sweep's `target` is the difference between the note and sweep frequencies and
`offset` ramps from 0 to it; without sweep flags bit 1 the base frequency becomes the sweep
key's; bit 2 keeps the sweep for the next note, else it is cleared. `CgbVoiceStart` triggers
with the base frequency (+0xC), not the per-frame one (+0x10); for square 1 it writes
instrument byte +8 (past the 8-byte record) to NR10, and the wave channel reloads wave RAM only
when the wave pointer differs from `gCgbWaveLoaded`. A note's first frame starts the channel;
later frames update NR51 routing (pan 0x40 both, below left, above right), the envelope (a new
NRx2 byte restarts the channel) and the duty list. A released wave voice decays in software,
`volume * (release + 0xE6) >> 9` per frame, written to NR32 through `gCgbWaveVolumes`, and
stops at zero.

**Sequence byte code** (keys 0-0x5F; lengths in ticks; `vl` = 1 byte, or 2 bytes
big-endian when bit 7 is set):

| byte | meaning |
|------|---------|
| `00–5F` | note, with the last length and velocity |
| `60–BF l v` | note `b - 0x60`, length `vl`, velocity (SE tracks also wait the length) |
| `C0` / `C1 vl` | wait the last wait / `vl` |
| `C2 p` `C3 p` `C4 p` | program, pan (0x40 centre), priority |
| `C5` / `C6` | tie on / off |
| `C7 b` | bank (`TrackSetBank`) |
| `C8` / `C9` | notes wait their length on / off |
| `CA b` | `gSoundSeqHook(track, b)` |
| `D0–DF k l [d]` | pitch sweep to key `k` over `l` (delay `d` if bit 0) |
| `E0`–`E3 b` | volume, bend (s8), bend range, +0x4C |
| `E4 vl` | player tempo |
| `E5`–`E7 b` | vibrato delay, speed (+0x14), depth (+0x18) |
| `E8` | sweep off |
| `E9 b` `EA b` | transpose, player volume (+0x40) |
| `F0 u16` / `F4 u16` | jump / call (little-endian offsets from the SE offset table) |
| `F8 n u16` | start track `n` of the player at the offset |
| `FF` | return, or end the track |

`TrackExecute` returns 1 when the track or its player is missing, 2 when the track ended
(`FF` with an empty call stack), else 0. `TracksInit`/`VoicesInit` write the work areas' list
pointers through `__attribute__((packed))` copies of the structs, hence the byte-wise stores.

## Version differences

- **GZLJ01 vs GZLE01**: same modules in the same order. Changes are mostly in game code
  (text/UI; the matcher pairs 337 vs 339 functions, with a few added or reshaped functions).
  Libraries are identical. One layout difference found while matching: the US HUD's left
  panel (`UpdateHudPanels`) is 5 tiles wide instead of 4 (`HUD_LEFT_WIDTH` in item_menu.c;
  the last entry of `gHudPanelTiles` differs to match).
- **GZLE01 vs GZLE01_ud** (`client_ud.bin`): byte-identical except one function,
  `ItemMenuUpdate` (`02024AB8`, JP `0201DEAC`), which grows by `0x234` bytes. Everything after it
  shifts by `0x234`, and `.rodata`/`.data` are the same apart from relocated pointers. The
  extra code is a **message test**. L (bit 9 of `gMain.newKeys`) toggles bit 5 of
  `gGameState+0x1E5`; while it is set, Up/Down step a message ID at `gGameState+0x142` by 1,
  Left/Right by 10 and R by 100, wrapping within 0-0x437 (the range `ShowMessage` accepts).
  The ID is shown as four digits at BG map `0x06006464`, in the rupee counter's digit tiles,
  and passed to `MsgStartSilent`, which shows it. "ud" is plausibly "USA debug". The game only
  loads `client_u.bin`.

## PAL (GZLP01_*)

The European disc has one client per language, `client_<n>.bin` (0 English, 1 German, 2
French, 3 Spanish, 4 Italian), which `d_a_agb.cpp` picks by `dComIfGs_getPalLanguage()`
together with `msg_LZ<n>.bin`. Each has a `client_<n>d.bin` twin that is never loaded and
differs from it exactly as `client_ud.bin` does from `client_u.bin` (the message test in
`ItemMenuUpdate`). All ten build from the US code (`VERSION_GZLE01`, or `VERSION_GZLE01_ud`
for the d builds) plus `VERSION_PAL` and `PAL_LANGUAGE`. The programs load at `0x0201B000`
(English, Spanish, Italian), `0x0201CB00` (German) and `0x0201BE00` (French).

Changes in every language:

- **GameState** grows by 8 bytes at `0x138` (the `#ifdef VERSION_PAL` block in the genstruct
  list): every field from `0x138` on is 8 bytes higher, `gGameState` is `0x248` bytes, and the
  IWRAM variables after it (`gGbaComDataStatus` ... `gObjs`) are `0x10` higher.
- **The examined selection is remembered.** In JP/US, A on a not yet examined marker makes
  Tingle think (`reactionTimer`), and only when that message is dismissed does `Mode1State2`
  call `ShowSelectionMsg` and reveal the marker, both for whatever is selected *then*: moving
  the cursor during the pause shows another marker's message and reveals the wrong hidden
  marker (or none). In PAL `ShowSelectionMsg` returns the message instead of showing it,
  `ExamineSelection` saves it with the selection's type and param (`pendingMsg`,
  `pendingType`, `pendingParam`, `pendingReveal`) when Tingle starts thinking, and
  `Mode1State2` shows and reveals the saved selection. An already examined selection also no
  longer plays voice clip 2 (sound ID 2) with its message.
- **Text**: message offsets count halfwords (`MsgStart`; the message data can pass 64 KiB);
  `TextDrawGlyph` and `TextDrawSmallGlyph` (accents) draw nothing below the box's last line
  (y > 0x44, or y = 0x44 with big text or x > 0x5A); message code `F3 11` sets the glyph
  width to 6 like `F3 10` instead of 8.
- `InitMapBgs` also copies BG map `0x06007AC0` over `0x06006840` in dungeons (the other label
  of the box that holds the floor number; the sea already copied `0x06007B40`).
- **Sound bank**: the voice clip sequences are reordered (`SOUND_SE` in sound.c).

Per language (`#if PAL_LANGUAGE == n`):

| | English | German | French | Spanish | Italian |
|--|--|--|--|--|--|
| title (`Mode0State0`) | 9 tiles, "Calling..." | 16, "Verbindungsaufbau..." | 13, "Appel en cours…" | 7, "LLAMANDO" | 16, "Chiamata in corso..." |
| floor box (`DrawFloorNumber`) | letter before a basement's digit, after an upper floor's | ground floor label; letter before both | ground floor label; basement letter after the digit | ground floor label | ground floor label |
| link-lost text canvas | 30 tiles | 25 | 29 | 28 | 26 |

- With a ground floor of their own (floor 1, presumably "EG", "RC", "PB", "PT"), the other
  languages show floor 2 as "1" and so on.
- **German and French** sound banks also drop voice clip 2 and the rupee counter's end sound
  (JP sound ID 9): the rupee counter stops silently, and sound IDs from 3 on are one to three
  lower (`SOUND_ID_SHIFT` 3, `SE_FREE_OFF`). Their EWRAM buffers below the program
  (`GBACOM_BUF_*`, the scroll tables) are `0x800`/`0x600` lower than the other languages'
  offset from the program base (`WORK_TOP`).
- **German** only: `Mode0State3` records whether the player's name ends in S, X or Z
  (`nameSibilant`), and in messages `0x13E` and `0x17F` the "s" after the name (message code
  `FB`) becomes an apostrophe: the genitive "Links" but "Max'".

## Answers to earlier open questions

- **Raw tail block**: it's the sound bank. `gSoundBankHeader` (`.rodata`, GZLJ01
  `0202CD20`) holds seven pointers into it, in this order (offsets GZLJ01 / US):

  | # | offset | contents |
  |---|--------|----------|
  | 0 | `+94C` / `+90C` | sample sets |
  | 1 | `+674` / `+634` | instrument banks |
  | 2 | end | song table: nothing there (`PlayerStartSong`, never posted) |
  | 3 | `+010` | SE sets |
  | 4 | `+93C` / `+8FC` | u16 bank -> sample set: `{0, 1, 0, 0}` |
  | 5 | `+944` / `+904` | song bank maps: 8 zero bytes (empty) |
  | 6 | `+000` | SE bank maps |

  Tables 0-3, 5 and 6 are u32 offset tables (from the table's start; the first offset / 4
  is the count), and each entry is again offset-based. SE bank maps: one map, u16
  `{0, 1}` (sequence bank -> global bank). SE sets: one set at `+014`, u16 offsets (from
  `+014`) of 57 sequences (US 56), then the byte code (see [Sound driver](#sound-driver)). Banks: two. A bank is u16 offsets (from the bank) per program (0 = none; bank 0 has
  76 slots, 32 used: 21 PCM, 4 square 1, 3 square 2, 4 noise; bank 1 has one), then
  envelopes (`{u16 frames; s16 level}` steps ramping linearly to `level`, 0x7FFF = full,
  `frames` 0xFFFF = hold), then 8-byte instrument records `{u8 type; u8 flags; u16 arg; u16
  envelope; u8 release; u8 base key}`. Type 0 is PCM (`arg` = sample index; the base key
  plays at the sample's rate), 1-4 the GB channels (`arg` = duty, or with flags bit 0 an
  offset to a per-frame duty list `{u16 n; u8 duty[n]}`; wave: offset to wave data), 0x10 a
  key split with fixed pans (`arg` -> `{u16 record; u8 pan; u8}` per key, indexed by key minus
  byte 4), 0x11 a drum kit (`arg` -> u16 sample index per key, default envelope, natural
  pitch), 0x12 key ranges (`arg` -> `{u8 top key; u8; u16 record}`). Flags bit 4: note length
  ignores the tempo offset. `release`: the released volume is multiplied by
  `(0xE6 + release) / 512` per frame. Bank 1 is just a drum kit on keys 0-6. Samples: two
  sets, each u32 offsets (from the set) to `{u32 length; u32 rate; u32 loop start; u32 loop
  end (0 = one-shot)}` + s8 data (what `PcmMixChannel` reads: `ldrsb`, 8.8 position). Set 0
  (bank 0): 16 effects (10512 or 48000 Hz). Set 1 (bank 1): 7 voice clips, 0x832-0x257A
  bytes at 10512 Hz, played by SE sequences 2-8 = sound IDs 0-6 on player 4.

  **US bank**: SE sequence 7 (sound ID 6: voice clip 6) is gone, so `gSoundTable` has 43
  entries and every SE index from 7 is one lower; `gSoundLoopTable` keeps its 11 entries
  (shifted indices). Set 1 has 6 clips, all but clip 2 with different lengths (probably localized).
  Set 0 drops sample 9 (0x121F bytes), and with it program 5 of bank 0, which only the
  fourth track of SE 0x16 (sound ID 0x12, US 0x11) used; the US sequence starts three tracks.
- **Packer "CpuSet copies half"**: it doesn't. The stub's ARM `svc #0x0C0000` is BIOS
  function `0x0C`, **CpuFastSet**, which always moves 32-bit words, so a word count is
  right. CpuFastSet rounds the count up to 8 words, so it reads up to 28 bytes past the end
  of the file and writes them past the tail. That's harmless.
- **`msg_LZ.bin` round-trip**: it's one plain stream, packed **VRAM-safe** (no distance-1
  matches) and zero-padded to 4 bytes. `lz77.compress(raw, min_dist=2)` plus padding
  reproduces both the JP and US files byte for byte.
- **`gMode1Unused_02023E38`** (`7D 7A 01 00 B2 B3 00 2B 00 00 00 FF`; still unexplained): it
  sits right after `main.c`'s last table (`gMainModeFuncs`, which ends at `02023E38`) and before
  `gDungeonMsgIds`, and is byte-identical in all three builds (US `0202AA94`/`0202ACC8`),
  even though the US replaced the Japanese font. No word anywhere in the program points into
  it or near it. What it is not:
  - Text in the message font (`gMode1BgTiles` + 0x2000): "7 4 い あ x y あ わ あ あ あ"; in the US
    font "(empty) (empty) A (space) ec ck (space) q", with `B2 B3` falling in the
    pre-drawn phrase tiles ("Check cursor position"). The same index into the start of the BG tiles or into the OBJ tiles
    (`gMode1ObjTiles`, which also has `0-9A-F` digit sprites) gives fragments of other graphics.
  - Sound IDs, loop IDs or message codes (`0x7D`, `0xB2` are past every table), a
    `struct SpritePart`/OAM entry (attribute bits make no sense), BGR555 colours, sea chart
    squares (0-48), a `struct GbaComFlags` (rupees would be 31357, wallet size 3), or one byte
    per item slot (nothing matches `gShopItemsInit`).
