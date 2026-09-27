#ifndef GUARD_JOY_COM_H
#define GUARD_JOY_COM_H

#include "global.h"

// JOY bus link with the GameCube: the GBA end of m_Do_gba_com.cpp. See docs/protocol.md.

#define JOYCOM_QUEUE_LEN 16

// Link handshake states (JoyCom.state)
enum
{
    JOYCOM_STATE_IDLE,       // waiting for a JOY reset command
    JOYCOM_STATE_RESET,      // reset seen, game code loaded into JOY_TRANS
    JOYCOM_STATE_CODE_SENT,  // GC read the game code, waiting for it to echo it back
    JOYCOM_STATE_CODE_OK,    // echo matched, waiting for the GC's context word
    JOYCOM_STATE_LINKED,     // context echoed: packets flow
};

// Wire words
#define JOYCOM_PACKET_START 0xFEFEFEFE
#define JOYCOM_CLOSE        0xEFEFEFEF
#define JOYCOM_IDLE         0xFFFFFFFF

struct JoyComPacket
{
    /*0x0*/ u32 *data;
    /*0x4*/ s32 size;
    /*0x8*/ u8 type;       // slot
    /*0xC*/ u32 checkSum;  // running sum of the words sent
};

struct JoyCom
{
    /*0x000*/ void *intrCopy;       // JoyComIntr copied into gMain.heap
    /*0x004*/ u32 context;          // the GC's random context word, echoed back
    /*0x008*/ u32 gameCode;         // sent after a JOY reset; the GC must echo it
    /*0x00C*/ void *const *buffers; // receive buffer per slot
    /*0x010*/ u8 *status;           // status per slot (gGbaComDataStatus)
    /*0x014*/ u32 *recvPtr;
    /*0x018*/ u32 *recvEnd;
    /*0x01C*/ u32 recvCheckSum;     // checksum word from the GC
    /*0x020*/ u32 recvSum;          // sum of the words received
    /*0x024*/ struct JoyComPacket queue[JOYCOM_QUEUE_LEN];
    /*0x124*/ u8 queueWrite:4;
              u8 queueRead:4;
    /*0x125*/ u8 queueCount;
    /*0x126*/ u8 numSlots;
    /*0x127*/ u8 sendState;
    /*0x128*/ u8 recvState;
    /*0x129*/ u8 recvType;
    /*0x12A*/ u8 idleFrames;        // frames without a JOY interrupt (JoyComMain)
    /*0x12B*/ u8 resetCount;        // JOY resets in a row; 10 -> hardReset
    /*0x12C*/ u8 otherIntrCount;    // clears resetCount after 4 other interrupts
    /*0x12D*/ u8 hardReset:1;       // JoyComMain will HardReset the GBA
              u8 state:3;
              u8 unk12D_4:1;
              u8 active:1;          // JoyComInit done, JoyComStop not yet called
              u8 hasContext:1;      // a context word has been accepted once
              u8 closed:1;          // JOYCOM_CLOSE received (cleared on every interrupt)
};

extern struct JoyCom gJoyCom;

void JoyComInit(void *const *buffers, u8 *status, u32 gameCode, u8 numSlots);
void JoyComStop(void);
void JoyComIntr(void);
bool32 JoyComMain(void);
bool32 JoyComSendDataSet(void *data, s32 size, u8 type, u32 checkSum);
bool32 JoyComStatusSendSet(u8 type);

#endif // GUARD_JOY_COM_H
