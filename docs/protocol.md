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
| 3 | GC->GBA? | never sent | `02014000` (0x100) |
| 4 GbaFlg | GBA->GC | `daAgb_c::FlagsRecv` | `02014100` |
| 5 mail | GC->GBA | `daAgbsw0_c` | `02014110` |
| 6 item | GBA->GC | `daAgb_c::GbaItemUse` | `02014140` |
| 7 effect | GC->GBA | reply to 6 | `02014150` |
| 8 switch | GBA->GC | `daAgb_c::SwitchOn` | `02014120` |
| 9 flags | GC->GBA | `daAgb_c::FlagsSend` (12 bytes) | `02014160` |
| 10 name | GC->GBA | player name (8 bytes) | `02014170` |
| 11 | GC->GBA? | never sent | `02014180` |
| 12 shop | GBA->GC | `daAgb_c::Shopping` | `02014290` |
| 13 item buy | GC->GBA | reply to 12 | `020142A0` |
| 14 | GC->GBA? | never sent | `02014190` (0x100) |
| 15 SE | GC->GBA | `daAgbsw0_c` bomb SE | `020142B0` |

Slots 3, 11 and 14 have GBA handlers, but no `mDoGac_SendDataSet`/`mDoGaC_SendEntry` in the
TWW decomp uses them, so they look like leftovers of cut features.

- **3** is the map-marker list, and it is copied: `Mode1RecvMarkers` (`020160C8`) takes a count
  byte, one more byte (`gGameState.unk133`, which also drives `unk104`), then 8-byte markers
  (`struct MapMarker`: `x`, `y`, a 6-bit `type` at +5) into `gGameState.markers`. It looks up
  the type-1 marker, and if the high byte of its x or y changed it resets a set of game-state fields
  (the US builds test the new position against `0x6FF` where JP tests the old one).
  `Mode0State3` waits for slot 3 before starting mode 1. `MainMode1` calls the handler each frame
  the slot has data, except in state 4 (`Mode1State4` calls it itself) and while a map change is
  pending; when it starts the fade out for a map change it resets slot 3 unread.
- **14** is a boot-time payload: `Mode0State3` copies 0xC4 bytes of it to IWRAM `0x03002FB0`
  (`gSlot14Data`): 49 `struct SeaChartEntry` `{x, y, big, square}`, one spot per sea chart
  square (`square` = row * 7 + column). `HighlightMapRooms` marks each spot of the rooms shown
  (2x2 tiles, 4x4 when `big`; square 25 only once `flags.unkA_4` is set), `MapIconCursor`
  names the square under the cursor, and `Func_0201B3A4` checks whether the player marker is
  within 8 pixels of its square's spot (32 when `big`). `MainMode1` doesn't touch it.
- **11** is copied by `MainMode1` (8 bytes, `CpuCopy16` to `gGameState.unk0F4`) and reset: a
  7x7 bitmap, one bit per sea chart square. `Func_0201B2C0` draws squares whose bit is clear
  darkened (palette 6), `MapIconCursor` only names a square whose bit is set, and
  `Func_0201B0F4` tests square 25 for one map tile. It looks like the "charted" squares.

In GZLE01 each buffer sits at the same offset from `prog_base`: `0x02010800` for slots 0/2
and `0x0201A800 + (JP address - 0x02014000)` for the rest.

## Where items may be used

Two sides gate a Tingle Tuner item:

- **GBA**: `flags.noMap` (GC `field_0xa_7`) is 0 only when `daAgb_c::field_0x670` is set
  (by `MapNoSet`, from the map code; cleared when the actor is created) and the stage type is
  dungeon, FF1, miniboss or sea. While it is 1, `MainMode1` fades out of the map screen and
  `Mode1State4` waits (key presses play the blocked sound). `MainLoop` sends
  `GbaFlg.unk3 = 1` ("active") only in mode 1 state 2 with no message or menu open. The GC
  copies it to `field_0x673`, and `modeMove` calls `GbaItemUse` only when `isActive()` (or
  for item `0x15`). On a relink the GBA goes through mode 2 to `Mode1State0`, which keeps
  the old map (`unk10E = 1`) when `noMap` is 0.
- **GC**: `uploadInitCheck` refuses rooms with `CheckAgbCom` or, off the sea, no map display;
  `GbaItemUse` answers items 3-5, 0x11 and 0x12 with a deny message in `CheckAgbHover` rooms
  while `mBombDeny` is set.

The disconnect glitch that lets items work in otherwise blocked places needs `noMap` = 0 (it
only works in dungeon/sea stages unless the GC check is patched), so what it bypasses is
presumably one of the GC-side checks. Not confirmed.
