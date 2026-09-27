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
turns into 6 after it is sent), so its meaning is open.

## Slots

See the enum in `include/gba_com.h`. The GBA receive buffer per slot is `gGbaComBuffers`
(GZLJ01 addresses below):

| Slot | Dir | GC source | GBA buffer (JP) |
|------|-----|-----------|-----------------|
| 0 message | GC->GBA | `uploadConnect`: `msg_LZ.bin` | `0200A000` (0xA000) |
| 1 control | GC->GBA | `dMap_c`, `modeDelete` | `02014130` |
| 2 map image | GC->GBA | `dMap_c` room map | `0200A000` (shared with 0) |
| 3 | ? | | `02014000` (0x100) |
| 4 GbaFlg | GBA->GC | `daAgb_c::FlagsRecv` | `02014100` |
| 5 mail | GC->GBA | `daAgbsw0_c` | `02014110` |
| 6 item | GBA->GC | `daAgb_c::GbaItemUse` | `02014140` |
| 7 effect | GC->GBA | reply to 6 | `02014150` |
| 8 switch | GBA->GC | `daAgb_c::SwitchOn` | `02014120` |
| 9 flags | GC->GBA | `daAgb_c::FlagsSend` (12 bytes) | `02014160` |
| 10 name | GC->GBA | player name (8 bytes) | `02014170` |
| 11 | ? | | `02014180` |
| 12 shop | GBA->GC | `daAgb_c::Shopping` | `02014290` |
| 13 item buy | GC->GBA | reply to 12 | `020142A0` |
| 14 | ? | | `02014190` (0x100) |
| 15 SE | GC->GBA | `daAgbsw0_c` bomb SE | `020142B0` |

In GZLE01 each buffer sits at the same offset from `prog_base`: `0x02010800` for slots 0/2
and `0x0201A800 + (JP address - 0x02014000)` for the rest.
