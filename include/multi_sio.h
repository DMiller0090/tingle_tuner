#ifndef GUARD_MULTI_SIO_H
#define GUARD_MULTI_SIO_H

#include "global.h"

// Nintendo's MultiSio library (GBA <-> GBA multi-player link), "MultiSioSync010316".
// Linked into the Tingle Tuner but never called. Field names follow the SDK's style;
// the originals aren't known.

#define MULTI_SIO_PLAYERS_MAX  4
#define MULTI_SIO_BLOCK_SIZE   0x18  // bytes per packet slot (0x14 used: header + 16 data)
#define MULTI_SIO_SYNC_DATA    0xFEFE
#define MULTI_SIO_TIMER_COUNT  0xABFB  // timer 3 reload for the parent's send interval

#define MULTI_SIO_TYPE_PARENT  0x08  // SIOCNT: SD high, ID 0

// SIOCNT (multi-player mode) followed by SIOMLT_SEND, as the SDK's SioMultiCnt.
struct SioMultiCnt
{
    u16 baudRate:2;
    u16 si:1;        // 0 = parent side of the cable
    u16 sd:1;        // 1 = all GBAs ready
    u16 id:2;
    u16 error:1;
    u16 enable:1;    // start / busy
    u16 unused8:4;
    u16 mode:2;
    u16 ifEnable:1;
    u16 unused15:1;
    u16 data;        // SIOMLT_SEND
};

#define REG_SIOMULTICNT (*(struct SioMultiCnt *)0x04000128)

struct MultiSioPacket
{
    /*0x00*/ u8 frameCounter;
    /*0x01*/ u8 recvErrorFlags;
    /*0x02*/ u16 checkSum;       // ~sum of the 10 halfwords
    /*0x04*/ u32 data[4];
    /*0x14*/ u16 unused[2];
};

struct MultiSioArea
{
    /*0x000*/ u8 type;              // 0 until connected, then MULTI_SIO_TYPE_PARENT or child
    /*0x001*/ u8 state;
    /*0x002*/ u8 connectedFlags;
    /*0x003*/ u8 recvSuccessFlags;
    /*0x004*/ u8 sendFlag;          // MultiSioSendDataSet filled nextSendBufp
    /*0x005*/ u8 recvFlag;          // a full packet arrived (MultiSioIntr)
    /*0x006*/ u8 startFlag;
    /*0x007*/ u8 hardError;
    /*0x008*/ u8 filler8[3];
    /*0x00B*/ u8 sendFrameCounter;
    /*0x00C*/ u8 fillerC[8];
    /*0x014*/ s32 sendBufCounter;   // halfword index into currentSendBufp
    /*0x018*/ s32 recvBufCounter;   // halfword index into currentRecvBufp, -1 = wait for sync
    /*0x01C*/ struct MultiSioPacket *nextSendBufp;
    /*0x020*/ struct MultiSioPacket *currentSendBufp;
    /*0x024*/ struct MultiSioPacket *currentRecvBufp;
    /*0x028*/ struct MultiSioPacket *lastRecvBufp;
    /*0x02C*/ struct MultiSioPacket *recvCheckBufp;
    /*0x030*/ struct MultiSioPacket recvBuf[3][MULTI_SIO_PLAYERS_MAX];
    /*0x150*/ struct MultiSioPacket sendBuf[2];
};

extern struct MultiSioArea gMultiSioArea;
extern const char MultiSioLibVer[];

void MultiSioInit(void);
void MultiSioStart(void);
void MultiSioStop(void);
u32 MultiSioMain(void *recvp);
void MultiSioSendDataSet(void *sendp);
u32 MultiSioRecvDataCheck(void *recvp);
void MultiSioVSync(void);
void MultiSioIntr(void);

#endif // GUARD_MULTI_SIO_H
