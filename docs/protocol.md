# GC <-> GBA link protocol

The GameCube side is `mDoGaC_agbCom_c` in `../tww/src/m_Do/m_Do_gba_com.cpp`, driving
`JUTGba` (JOY bus commands: reset `0xFF`, status `0x00`, read `0x14`, write `0x15`). The GBA
side is `JoyCom` (`joy_com.c`, the transport) plus `GbaCom` (`gba_com.c`, the per-slot API the
game uses). Names and layouts are in `include/joy_com.h` and `include/gba_com.h`.

Every JOY transfer carries one 32-bit word. The GC always does a *write* (GBA `JOY_RECV`)
and then a *read* (GBA `JOY_TRANS`), so each direction moves one word per cycle. The GBA
handles everything in `JoyComIntr` (serial IRQ, run from an IWRAM copy in `gMain.heap`).

## Boot and handshake

After JOY-boot the packer has already done one game-code exchange. The program then starts
the link itself: `InitSystem -> GbaComInit -> JoyComInit(gGbaComBuffers,
gGbaComDataStatus, gMain.gameCode, 16)` puts RCNT in JOY mode and enables the serial IRQ.

| GC (`m_Do_gba_com.cpp`) | GBA `JoyComIntr` | `gJoyCom.state` |
|-------------------------|------------------|-----------------|
| `doReset` (Connect / ProbeCheck0) | JOY reset: `JOY_TRANS = gameCode`, set JOYSTAT general flag (0x20) | -> 1 `RESET` |
| `doGetStatus` expects 0x28 (CodeExchange0) | | |
| `doRead` -> code (CodeExchange1 checks `"GZLJ"`/`"GZLE"`) | send IRQ | -> 2 `CODE_SENT` |
| `doGetStatus` 0x20, `doWrite(code)` (CodeExchange2/3) | recv IRQ, word == gameCode: set JOYSTAT 0x10 | -> 3 `CODE_OK` |
| `doGetStatus` 0x30, `doWrite(context)` (CodeExchange4) | recv IRQ: store `context`, echo it in `JOY_TRANS`, clear all slot statuses and the queue | -> 4 `LINKED` |
| `doRead` (ContextRead/Check) compares the echo | | |

The context is `field_0x128[0].field_0x4.U32` on the GC: a random 15-bit value plus the
port number in the low 2 bits, byte-swapped. On a reconnect (`hasContext` already set),
the GBA accepts only the same context word; a different one drops it back to `IDLE`. That
is how the GC tells a reconnect (`mReConnect = 1`) from a fresh GBA.

Other JOY events:

- 10 JOY resets in a row (`resetCount`, cleared after 4 other interrupts) set `hardReset`.
  The next `JoyComMain` calls `JoyHardReset` (BIOS HardReset if no cartridge and RCNT is
  still in JOY mode), then `JoyComStop`. This is how `mDoGaC_GbaReset` retries reboot the GBA.
- `JoyComMain` runs once per frame (`MainLoop -> GbaComMain`). When linked, 10+ frames
  with no JOY interrupt (`idleFrames`) re-initialise the link but keep `context`, so the GC
  can reconnect.
- `GbaComMain` returns "linked" = `active && state == LINKED`, the GBA's counterpart of
  `mDoGaC_GbaLink`.

## Packets

Once linked, both directions use the same framing (GC `mDoGaC_GbaWrite` / `mDoGaC_ReadResult`,
GBA `JoyComIntr` recv/send state machines). The GC byte-swaps header words with
`BigLittleChange`, so every word is a little-endian u32 on the GBA. Data words go through
unswapped, so the buffer keeps its byte order.

| Word | Content |
|------|---------|
| 0 | `0xFEFEFEFE` start |
| 1 | slot (`type`, 0–15) |
| 2 | sender's status for the slot: "data follows" (GC sends 3, GBA sends 7), or just a status update |
| 3 | size in bytes (only if data follows) |
| 4.. | data words (size rounded up to 4) |
| last | checksum: 32-bit sum of the data words |

A packet with no data (GC `mDoGaC_StatusSendSet`, GBA `JoyComStatusSendSet`: size 0) is a
status update. It ends after word 2, and the receiver copies the value into its own status
byte for that slot. This is the acknowledgement path: when a receiver is done with the data
it resets the slot to 0 and sends a status packet. The sender then sees 0 and may send again.

Between packets the GC writes `0xFFFFFFFF`. It writes `0xEFEFEFEF` when it shuts the link
down (queue empty with `field_0x5 == 1`, followed by `mDoGaC_ComStop`), which sets
`gJoyCom.closed` for one interrupt (`GbaComIsClosed`). The GBA only loads the
next outgoing word when the GC has read the previous one (JOYSTAT bit 3 clear), and only
starts a packet when its queue (`JoyComSendDataSet`, 16 entries) is non-empty.

A receive checksum mismatch marks the slot `ERROR` (9) and queues a status packet, so the
sender learns about it. The GC does the same, and also closes the link.

## Slot status lifecycle (GBA)

Sending (GBA -> GC), e.g. `GbaComSendDataSet(&item, 4, GBACOM_SLOT_ITEM, 0)`:

`IDLE/5` —`JoyComSendDataSet`→ `QUEUED` (6) —start of packet→ `SENDING` (7) —checksum sent→
`SENT` (8) —GC status packet 0→ `IDLE`.

Receiving (GC -> GBA):

`IDLE` —GC packet, status word 3→ `RECEIVING` (3) —checksum OK→ `RECEIVED` (4)
(`GbaComRecvStatusCheck`) —`GbaComRecvDataGet` / `GbaComDataStatusReset`→ `IDLE`, plus a
status packet so the GC's `mDoGaC_SendEndCheck` sees 0.

`GbaComRequestData` (status 1, unused) queues a bare status packet. The GC stores 1 and
treats the slot as sendable (`mDoGaC_SendStatusCheck` accepts 0 or 1). Status 5 is only
ever set by a GC status word, and nothing in `m_Do_gba_com.cpp` produces it (GC status 5
turns into 6 after it is sent). The only GBA reader, `GbaComStatusIs5`, has no callers, so
status 5 is dead on both sides.

## Slots

See the enum in `include/gba_com.h`. The GBA receive buffer per slot is `gGbaComBuffers`
(GZLJ01 addresses below):

| Slot | Dir | GC source | GBA buffer (JP) |
|------|-----|-----------|-----------------|
| 0 message | GC->GBA | `uploadConnect`: `msg_LZ.bin` | `0200A000` (0xA000) |
| 1 control | GC->GBA | `dMap_c`, `modeDelete` | `02014130` |
| 2 map image | GC->GBA | `dMap_c` room map | `0200A000` (shared with 0) |
| 3 markers | GC->GBA | `dMap_c::mapBufferSendAGB_commonCursor` (map points) | `02014000` (0x100) |
| 4 GbaFlg | GBA->GC | `daAgb_c::FlagsRecv` | `02014100` |
| 5 mail | GC->GBA | `daAgbsw0_c` | `02014110` |
| 6 item | GBA->GC | `daAgb_c::GbaItemUse` | `02014140` |
| 7 effect | GC->GBA | reply to 6 | `02014150` |
| 8 switch | GBA->GC | `daAgb_c::SwitchOn` | `02014120` |
| 9 flags | GC->GBA | `daAgb_c::FlagsSend` (12 bytes) | `02014160` |
| 10 name | GC->GBA | player name (8 bytes) | `02014170` |
| 11 visited | GC->GBA | `dMap_c::mapBufferSendAGB_ocean` (8 bytes) | `02014180` |
| 12 shop | GBA->GC | `daAgb_c::Shopping` | `02014290` |
| 13 item buy | GC->GBA | reply to 12 | `020142A0` |
| 14 chart points | GC->GBA | `dMap_c::mapAGBSendIslandData` (0xC4 bytes) | `02014190` (0x100) |
| 15 SE | GC->GBA | `daAgbsw0_c` bomb SE | `020142B0` |

Slots 3, 11 and 14 come from `d_map.cpp` functions that are still asm (`/* Nonmatching */`) in
the TWW decomp, so a search of its C for `mDoGac_SendDataSet` misses them (read
`build/GZLJ01/asm/d/d_map.s`). `daAgb_c::uploadConnect` sends 14 right after the name (10);
`dScnPlay`'s draw calls `dMap_c::mapBufferSendAGB(3)` every frame outside the menu, which sends
3 (`mapBufferSendAGB_ocean` or `_dungeon`, both through `_commonCursor`; `_dungeon` only once
the map image went out, `mAGBMapSendStatus` 4) and on the sea also 11.

- **3** is the map-marker list (`mAgbSendBuf`), and it is copied: `Mode1RecvMarkers`
  (`020160C8`) takes a count byte, the GC's `agbIsActive()` (`gGameState.gcActive`, which also
  drives `active`), then 8-byte markers (`struct MapMarker`) into `gGameState.markers`. The GC
  builds them in `setGbaPoint_ocean`/`_dungeon`: `x`, `y` (byte-swapped), the angle's high byte,
  the type (6 bits; enemies add a 2-bit highlight), then two bytes of the point (`param` and
  `gbaName`). Marker 0 is the player (type 1), marker 1 the Tingle Tuner cursor (type 3; without
  a cursor point, a copy of the player's), then up to 14 others. `Mode1RecvMarkers` looks up the
  player's marker, and if the high byte of its x or y changed (Link entered another sea chart
  square) it resets the examined flags, the selection and the pending switch record, and
  schedules the square's name (`squareMsgPending`; the US builds test the new position against
  `0x6FF` where JP tests the old one).
  `Mode0State3` waits for slot 3 before starting mode 1. `MainMode1` calls the handler each frame
  the slot has data, except in state 4 (`Mode1State4` calls it itself) and while a map change is
  pending; when it starts the fade out for a map change it resets slot 3 unread.
- **14** is a boot-time payload: `Mode0State3` copies 0xC4 bytes of it to IWRAM `0x03002FB0`
  (`gChartPoints`): 49 `struct SeaChartEntry` `{x, y, big, square}`, one spot per sea chart
  square (`square` = row * 7 + column), from the stage's fmap check points.
  `HighlightMapRooms` marks each spot of the rooms shown (2x2 tiles, 4x4 when `big`; square 25,
  the Tower of the Gods, only once `flags.towerRaised` is set), `MapIconCursor` names the square
  under the cursor, and `UpdateSelection` checks whether the cursor's marker is within 8 pixels
  of its square's spot (32 when `big`). `MainMode1` doesn't touch it.
- **11** is copied by `MainMode1` (8 bytes, `CpuCopy16` to `gGameState.visitedSquares`) and
  reset: a 7x7 bitmap, one bit per sea chart square. `DarkenUnvisitedSquares` draws squares
  whose bit is clear darkened (palette 6), `MapIconCursor` only names a square whose bit is set, and
  `UpdateChartOverlay` tests square 25 for one map tile. The GC sets bit n when
  `dComIfGs_isSaveArriveGridForAgb(n)`: the squares Link has arrived at.

In GZLE01 each buffer sits at the same offset from `prog_base`: `0x02010800` for slots 0/2
and `0x0201A800 + (JP address - 0x02014000)` for the rest.

## Where items may be used

Two sides gate a Tingle Tuner item:

- **GBA**: `flags.noMap` (GC `field_0xa_7`) is 0 only when `daAgb_c::field_0x670` is set
  (by `MapNoSet`, from the map code; cleared when the actor is created) and the stage type is
  dungeon, FF1, miniboss or sea. While it is 1, `MainMode1` fades out of the map screen and
  `Mode1State4` waits (key presses play the blocked sound). `MainLoop` sends
  `GbaFlg.active = 1` only in mode 1 state 2 with no message or menu open. The GC
  copies it to `field_0x673`, and `modeMove` calls `GbaItemUse` only when `isActive()` (or
  for item `0x15`). On a relink the GBA goes through mode 2 to `Mode1State0`, which keeps
  the old map (`mapLoaded = 1`) when `noMap` is 0.
- **GC**: `uploadInitCheck` refuses rooms with `CheckAgbCom` or, off the sea, no map display;
  `GbaItemUse` answers items 3-5, 0x11 and 0x12 with a deny message in `CheckAgbHover` rooms
  while `mBombDeny` is set.

**The disconnect glitch** (watched live in GZLJ01 `GanonA`, 2026-09-27). `GanonA` is a
dungeon-type stage, but `field_0x670` stays 0 there: `MapNoSet` is normally only called from
`dMap_c::mapAGBSendMapMain` when a map image is sent (send status case 2), and rooms whose map
can't be displayed never get there. So the GC sends `noMap` = 1 and the GBA waits in
`Mode1State4`. It takes two reconnects:

1. **First reconnect**: `mDoGaC_ContextCheck` sees the known context and sets `mReConnect`
   (`field_0x128[0].field_0x0` was already 1). `mapAGBSendMapMain` then calls
   `agbMapNoSetCall` directly (d_map.cpp ~1511), skipping the map-display check:
   `MapNoSet(8, 1, 0)`, `field_0x670` = 1, and `noMap` = 0 from the next `FlagsSend` (~0.1 s
   after the relink). But the GBA re-entered mode 1 while `noMap` was still 1, so
   `Mode1State0` set `mapLoaded = 0` ("no map loaded"). In a dungeon (`dungeonNo` 17 here),
   `Mode1State4` only moves on once `mapLoaded` is set by a received map image (slot 2), which
   this room never sends. The GBA stays stuck on the wait screen.
2. **Second reconnect**: the GBA goes through mode 2 into `Mode1State0` again. Now `noMap` = 0,
   so it sets `mapLoaded = 1` (it keeps the old map) and `mapImageMode = inDungeon`. It runs the intro
   (`Mode1State1`) into state 2, sends "active" (~7 s after the relink), and items go through
   (`GbaItemUse`, e.g. a Tingle Bomb).

So the GC half of the bug is the reconnect path calling `MapNoSet` unconditionally. The GBA
half is `Mode1State0` trusting `noMap` = 0 to mean a map is already loaded.

**Slow potions in the glitched state.** Effect8 (items 8-10) drops the potion sprite from
0x30 px above the map view (`bgOfs[3][1]`) to 0x18 px above the player marker at 1 px per
2 frames (state 4), then flies it back up at 1 px per frame (state 7). In the glitched state
the map buffer is zeroed, so the view clamp (`0 .. height - 120` with height 0) puts
`mapY` at -120 and the view at -128. The player marker is frozen too: markers (slot 3)
only arrive with the map send, which never runs in these rooms, so the Tuner keeps the one
from the last room that sent them. The effect then takes 3 * (marker.y + 152) frames
(2x down, 1x up). Measured in `GanonA` (marker y 82): 7.8 s + 4.0 s, the same wherever Link
walked. Normal: 3.1 s + 1.6 s (view y 107, marker y 175: 92 px, 3.07 s predicted).

**Which map survives.** `InitMapView` zeroes the whole map/message buffer when it runs with
`mapImageMode != 0` (dungeon) and `mapLoaded == 0` (no map), which is what the first of two GT
reconnects does. Disconnecting once in the overworld instead leaves `mReConnect` set for good,
so entering GT already sends `noMap` = 0. The fade out on entering GT still runs `InitMapView`
with the overworld's `mapImageMode` = 0 (no wipe), and the single GT reconnect then uses the last map
image the Tuner received (measured: Windfall's 392x472 map drawn in GT, view (256, 107), marker
(324, 175), potion 3.07 s + 1.58 s, i.e. Windfall speed). With a real map the view is clamped at
`mapY` >= 0, so a stale marker within 60 px of that map's top edge shortens the effect to
`marker.y + 32` px: faster than normal.

## Short link drops

A drop of the link shorter than about 3 GC frames doesn't look like a disconnect to either side,
but it does lose packets:

- **GBA**: `JoyComMain` only restarts the link (and `GbaComMain` only enters mode 2, the
  "link lost" screen) after 10 frames without a JOY interrupt (`idleFrames > 9`).
- **GC**: a failed transfer sets `field_0x4`, `mDoGaC_Connect` calls `mDoGaC_ComClose` (every
  slot to status 9) and probes again once per frame. `mDoGaC_GbaLink` stays 1 while the context
  is known and fewer than 3 probes have run since the link was last up (`field_0x10e < 3`), so
  `daAgb_c` carries on. The probe resets the GBA (`JOYCNT_RESET`); the GBA's `CODE_OK` step
  sets all its slots idle and empties its send queue, and `mDoGaC_ComRestart` empties the GC's
  (`field_0x9`/`0xa`). `mDoGaC_ContextCheck` sets `mReConnect`.

Whatever either side had queued is gone, and neither side resends it. Only when 3 or more probes
fail does `mDoGaC_GbaLink` return 0 for a frame; retail `daAgb_Execute` then clears the item
timer `field_0x65c` and `field_0x66b`, cuts a running hover or shield to 10 frames, and resets the
cursor.

**Skipping the Tingle Bomb wait** (GZLJ01, watched live 2026-09-28). After a Tingle Bomb two
timers block items: the GC's `field_0x65c` = 120 (4 s; `modeMove` calls `onActive` only at 0,
and `GbaItemUse` refuses everything but 0x15 while inactive), and the GBA's 180-frame
`EffectBombBlast` with `itemsEnabled` = 0, which START (the item menu) and B need. Retail
`UseItem` doesn't clear `itemsEnabled` for a bomb; that happens when the GC's reply (slot 7,
effect 2) arrives. Drop the link after the GC has read the request but before the reply reached
the GBA, and the GC uses the bomb (bomb spawned, rupees charged, timer 120) while the GBA never
locks. Measured: GC received the request at 12.312 s, transfer error at 12.329, `GbaItemUse` ran at
12.333 (reply queued), `ComClose` at 12.339 wiped it; 2 probes, GBA `idleFrames` peaked at 5,
linked again at 12.441. The green Ting was started at 12.912. The GC timer is untouched but
doesn't matter: a Ting's `StartEffectIntro` animation is 333 GBA frames (plus the message) before
it sends 0x14, which went out at 18.739, after the timer ran out (16.337), and was accepted. The window
is from the GC reading the request to the reply landing, about one GC frame; earlier loses the
request (no bomb), later lets the reply through.

**Clearing the GC timer too** (watched live 2026-09-28). A drop long enough for the third probe
to fail makes `mDoGaC_GbaLink` return 0 for one frame, and `field_0x65c` went 117 -> 0 with the
bomb reply lost as well, while the GBA's `idleFrames` peaked at 7 (no link-lost screen). The
window is narrow: the probes run at the GC's 30 fps, so the same hold length gives 2 or 3 failed
probes depending on its phase, and one attempt reached 3 probes at `idleFrames` 8 but hit 10
before the handshake finished (mode 2). It also costs a map reload: with `GbaLink` 0,
`dMap_c::mapAGBSendMapMain` resets `mAGBMapSendStatus` (d_map.cpp ~1497), the GBA fades out
(`Mode1State3`), waits for the map (`Mode1State4`) and fades back in: about 214 GBA frames
(3.6 s), about as long as the 180-frame blast it replaces. So it doesn't speed up bomb chains.

**Striped map after a disconnect at a stage transition** (GZLJ01, `M_Dra09` room 9, watched
2026-09-28). The map screen showed identical columns of stray tiles. The GBA's map image had an
all-zero header (width, height, `tilesSize`, `mapWidth` 0) with `tiles` and `map` both
`0x0200A000`, and from offset 0x1BC on it matched the GC's image for the room byte for byte
(0x5580 bytes, `mAGBMapSendStatus` 4). So the image was arriving when the GBA wiped the buffer:
`InitMapView` clears all 0xA000 bytes when `mapImageMode` != 0 and `mapLoaded` == 0, which is what
`Mode1State0` does on a relink while `noMap` is still 1. The JOY interrupt kept writing the rest in
place, `LoadMapImage` then turned the zeroed header's offsets into the buffer's own address, and
with `mapWidth` 0 every row of the view draws the same map entries from the header, over whatever
tiles were already in VRAM. The GC resends the map after its link check fails, so the first
words of the image can land before the GBA gets from mode 2 to `Mode1State0`. The next map change
loads a proper image.

**Other items.** The drop only removes a GBA wait that starts when the reply arrives and lasts
longer than the GC's own lock:

| Item (request) | GBA wait starts | Lost reply |
|---|---|---|
| Tingle Bomb (2) -> effect 2 / 15 | on the reply (180 frames) | skips it; GC timer 120 |
| Tingle Bomb on Link (2) -> effect 14 | on the reply (300 + 180 frames) | skips it, but GC timer 270 (9 s) refuses a 0x14 sent within 9 s |
| A, nothing selected (16) | on the reply (`StartLookEffect`, 120 frames) | skips it; no GC timer |
| Ting 8-10 (0x14, then 8-10) | at `UseItem` (`StartTingEffect`) | nothing to skip; losing the 8-10 reply leaves the GBA at `effectStep` 5 until a map change |
| Balloon / shield (0x11 / 0x12, then 3 / 4) | on the 0x11 / 0x12 reply | losing it cancels the item (0x11 / 0x12 do nothing on the GC); the effect then runs off the GC's `flags.timer`, so losing the 3 / 4 reply changes nothing |
| Roulette (5) | on the reply | losing it cancels the roll (nothing charged yet); losing a later 0x13 reply leaves the GBA at `effectStep` 5 |
| Shop (slot 12) | dialogue waits for slot 13 | lost reply leaves the dialogue stuck (keys masked) until a map change |

## Unlocking discounts through a missing message download

The Tuner's unlockables are the shop (slot 6, item 7 instead of 11), the balloon/shield/roulette
discount and the Ting discount (roulette and the green and blue Tings only need `flags.hasMagic`).
The GBA sets them with the message commands `F3 05`, `F3 00` and `F3 01`, which only the JP
messages 0x3D2, 0x3B4 and 0x9E/0x3BC contain; these are mailed by `agbF` actors (sea room 44,
Windfall, Forsaken Fortress 2/3 room 1). `MainLoop` reports the three bits in `GbaFlg` every
frame, and `daAgb_c::FlagsRecv` turns a set bit into a permanent event flag. Nothing checks that
the bit came from those messages.

**Garbled text** (GZLJ01, watched live 2026-09-28). Disconnect during the connection phase after
the JOY boot image has arrived but before the message data (slot 0): `Mode0State1` gives up after
120 frames and goes to mode 2, and on the relink `Mode2State1` goes straight to mode 1. The
message area at `0x02000000` still holds the boot image, which is `client.bin` except for the
three header bytes the BIOS patches (0x00, 0xC4, 0xC5), so every message reads `client.bin` as
text, the same way every time. The report byte stays 0 (`Mode0State3` never copied the GC's
flags), which is harmless since the GC never clears them. In that state 240 of the 1080 message
IDs freeze the Tuner (an `F3` handler index >= 0x13), and six run unlock commands:

| Message | Runs | Normally shown by |
|---|---|---|
| 0x263 | `F3 00` | `agbA` in `Siren` (Tower of the Gods) room 8 at (-806, -295, -12934), cursor hit, gone once switch 0x0D is set |
| 0x26A | `F3 00` | `agbF` in `Siren` room 10 (condition 0x42: in a room with a `daNpc_Os`) |
| 0x11E | `F3 01` | `ShowSelectionMsg` for a map point with `gbaName` 0xAF, which nothing on the GC assigns (the object table tops out at 0x41) |
| 0x11F | `F3 00` | `ShowSelectionMsg` for a map point with `gbaName` 0xB0, likewise unused |
| 0x20E, 0x349 | `F3 00` | no source found |

No reachable message runs `F3 05` or `F3 01`, so only the balloon discount can be unlocked this way. A later disconnect in the same phase gives the same state: the message data is unpacked in one BIOS call, so the area holds either the boot image or the real text. Measured with 0x263: the mail
arrived as the cursor reached the switch, A started the message, 1.7 s later the report byte
went to 0x10 (balloon discount) and 44 ms after that the GC's `mFlags` byte 7 showed
`UNLOCK_TINGLE_BALLOON_DISCOUNT` set. It is kept once the game is saved.

### What the garbled text can run

The discount unlocks above are a special case of a wider question: with `0x02000000` holding the
raw boot image, `MsgStep` (`message.c`) executes every control byte `0xF2`-`0xFF` it walks over
before drawing, and the only one that transfers control is `F3 xx`, an *unchecked* table jump:

```c
else if (ch == 0xF3)   // call a handler
    gMsgCmdFuncs[*++gGameState.msgPtr]();
```

`gMsgCmdFuncs` (`0x020243A4`) has 19 real entries (`00`-`12`). Any `xx >= 0x13` reads a word past
the table and calls it. Because the jump target is fixed image data, this is never arbitrary code
injection: an index only ever reaches one of a fixed set of addresses, and the reachable set is
bounded by which `F3 xx` bytes actually appear in the boot image at each message's offset. Reading
the table words past index 0x12:

- `0x13`-`0x2E` land in the sprite/animation tables that follow `gMsgCmdFuncs` (`gTingleAnims`,
  `gTingleAnim*`, `gTingleSprite`) — data words, not code, so calling them hangs. This is most of
  the 240/1080 IDs that freeze.
- `0x2F`-`0x4F` are small values or IO-range junk — hang.
- `0x50`-`0x70` land on **real Thumb functions**, because the next image tables (`gItemEffectFuncs`,
  `gMode2StateFuncs`, `gObjFuncs`, `gObjBurstStateFuncs`) are themselves arrays of valid pointers.
  This is the only band that does something other than freeze.

Scanning all 0x438 message offsets in the boot image for the first out-of-range `F3 xx` each one
reaches (a message stops mattering after that call diverts control), only four such calls are ever
reached from real message data:

| Runs (`F3 xx`) | Handler | Message IDs (garbled state) |
|---|---|---|
| `F3 56`, `F3 60` | `EffectWaitTimer` (`effect.c`) | 0xD0, 0x158, 0x163, 0x1B8, 0x1DF, 0x32F, … (21) |
| `F3 58` | `Effect8` (`effect.c`) | 0x81, 0x8F, 0x99, 0xA1, 0x18F, 0x23D, 0x3A0, 0x3BC |
| `F3 5F` | `EffectBombBlast` (`effect.c`) | 0x47, 0x124, 0x1FC |

So the realistic outcome space of a garbled textbox is **freeze (most IDs), unlock the balloon
discount (0x263 etc., via the in-range `F3 00`), or enter one of `EffectWaitTimer` /
`EffectBombBlast` / `Effect8` out of sequence** with whatever `effect[]`, `effectStep`,
`effectDelay` and marker state happened to be live:

- `Effect8` is the one worth testing. Normally it is the items-8-10 (Tings) drop-in that issues
  `SendItemUse(0x14, …)` and `SendItemUse(idx + 8, price)`; entered cold, the branch it takes and
  the item index/price it sends depend on `effect[0]`/`effectStep`, a possible path to an
  item-use the menu never authorised. Candidate test: JP, reach the garbled state, trigger message
  0x81 with slot-7/marker state set, and watch the GC's `SendItemUse` args and
  `gGameState.itemsEnabled`.
- `EffectWaitTimer` and `EffectBombBlast` can call `EndItemEffect` (`itemsEnabled = 1`) — another
  route to clearing the bomb lockout independent of the GC's slot-7 reply.

No garbled message reaches `Effect5/7/11/12/14`, the `Mode2State*` or `ObjBurst*` entries, or the
free-bomb `MsgCmd12` (`F3 12` is in range and needs the real, not garbled, text).
