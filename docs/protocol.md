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
