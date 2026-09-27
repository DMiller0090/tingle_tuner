#include "global.h"
#include "gba/syscall.h"
#include "multi_sio.h"

// Nintendo MultiSio (GBA <-> GBA link, parent drives transfers from timer 3). Unused by
// the Tingle Tuner, which talks to the GameCube over JoyCom instead.

extern u32 gMultiSioUnk_03005370;

const char MultiSioLibVer[] = "MultiSioSync010316";

void MultiSioInit(void)
{
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = 1;
    REG_RCNT = 0;
    REG_SIOCNT = SIO_MULTI_MODE;
    REG_SIOCNT |= SIO_IF_ENABLE | SIO_115200_BPS;
    CpuFill32(0, &gMultiSioArea, sizeof(gMultiSioArea));
    gMultiSioArea.sendBufCounter = 12;
    gMultiSioArea.recvBufCounter = 12;
    gMultiSioArea.nextSendBufp = &gMultiSioArea.sendBuf[0];
    gMultiSioArea.currentSendBufp = &gMultiSioArea.sendBuf[1];
    gMultiSioArea.currentRecvBufp = gMultiSioArea.recvBuf[0];
    gMultiSioArea.lastRecvBufp = gMultiSioArea.recvBuf[1];
    gMultiSioArea.recvCheckBufp = gMultiSioArea.recvBuf[2];
    REG_IME = 0;
    REG_IE |= INTR_FLAG_SERIAL;
    REG_IME = 1;
}

void MultiSioStart(void)
{
    if (gMultiSioArea.type)
        gMultiSioArea.startFlag = TRUE;
}

void MultiSioStop(void)
{
    REG_IME = 0;
    REG_IE &= ~(INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL);
    REG_IME = 1;
    REG_SIOCNT = SIO_MULTI_MODE | SIO_115200_BPS;
    REG_TM3CNT = MULTI_SIO_TIMER_COUNT;
    REG_IF = INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL;
    gMultiSioUnk_03005370 = gMultiSioArea.startFlag = FALSE;
}

u32 MultiSioMain(void *recvp)
{
    struct SioMultiCnt siocnt = REG_SIOMULTICNT;

    switch (gMultiSioArea.state)
    {
    case 0:
        if (siocnt.id == 0)
        {
            if (siocnt.sd != 1 || siocnt.enable != 0)
                break;
            if (siocnt.si == 0 && gMultiSioArea.sendBufCounter == 12)
            {
                REG_IME = 0;
                REG_IE &= ~INTR_FLAG_SERIAL;
                REG_IE |= INTR_FLAG_TIMER3;
                REG_IME = 1;
                REG_TM3CNT = MULTI_SIO_TIMER_COUNT;
                REG_SIOMULTICNT.ifEnable = 0;
                REG_IF = INTR_FLAG_TIMER3 | INTR_FLAG_SERIAL;
                gMultiSioArea.type = MULTI_SIO_TYPE_PARENT;
            }
        }
        gMultiSioArea.state = 1;
    case 1:
        MultiSioRecvDataCheck(recvp);
        break;
    }

    gMultiSioArea.sendFrameCounter++;
    return gMultiSioArea.recvSuccessFlags | (gMultiSioArea.type == MULTI_SIO_TYPE_PARENT) << 7
         | (gMultiSioArea.connectedFlags << 8)
         | (gMultiSioArea.hardError != 0) << 12
         | (siocnt.id >= 4) << 13;
}


void MultiSioSendDataSet(void *sendp)
{
    s32 sum = 0;
    s32 i;

    gMultiSioArea.nextSendBufp->frameCounter = gMultiSioArea.sendFrameCounter;
    gMultiSioArea.nextSendBufp->recvErrorFlags = gMultiSioArea.connectedFlags ^ gMultiSioArea.recvSuccessFlags;
    gMultiSioArea.nextSendBufp->checkSum = 0;
    CpuCopy32(sendp, gMultiSioArea.nextSendBufp->data, sizeof(gMultiSioArea.nextSendBufp->data));
    for (i = 0; i < 10; i++)
        sum += ((u16 *)gMultiSioArea.nextSendBufp)[i];
    gMultiSioArea.nextSendBufp->checkSum = ~sum;
    gMultiSioArea.sendFlag = TRUE;
}

// Nearly matches: the packet pointer and the checksum get each other's registers.
// u32 MultiSioRecvDataCheck(void *recvp)
// {
//     vu32 zero = 0;
//     struct MultiSioPacket *swap;
//     u8 recvFlag;
//     s32 i;
//
//     REG_IME = 0;
//     swap = gMultiSioArea.recvCheckBufp;
//     gMultiSioArea.recvCheckBufp = gMultiSioArea.lastRecvBufp;
//     gMultiSioArea.lastRecvBufp = swap;
//     recvFlag = gMultiSioArea.recvFlag;
//     gMultiSioArea.recvFlag = FALSE;
//     REG_IME = 1;
//     gMultiSioArea.recvSuccessFlags = 0;
//
//     if (recvFlag)
//     {
//         for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++)
//         {
//             s32 sum;
//             u16 *packet;
//             u32 *data;
//             s32 j;
//
//             packet = (u16 *)&gMultiSioArea.recvCheckBufp[i];
//             sum = 0;
//
//             for (j = 0; j < 10; j++)
//                 sum += packet[j];
//             data = ((struct MultiSioPacket *)packet)->data;
//             if ((s16)sum == -1)
//             {
//                 CpuCopy32(data, (u8 *)recvp + i * 16, 16);
//                 gMultiSioArea.recvSuccessFlags |= 1 << i;
//             }
//             CpuFill32(0, data, 16);
//         }
//     }
//     gMultiSioArea.connectedFlags |= gMultiSioArea.recvSuccessFlags;
//     return gMultiSioArea.recvSuccessFlags;
// }
INCLUDE_ASM(MultiSioRecvDataCheck);

void MultiSioVSync(void)
{
    struct MultiSioPacket *swap;

    if (gMultiSioArea.state && gMultiSioArea.startFlag)
    {
        gMultiSioArea.recvBufCounter = -1;
        swap = gMultiSioArea.lastRecvBufp;
        gMultiSioArea.lastRecvBufp = gMultiSioArea.currentRecvBufp;
        gMultiSioArea.currentRecvBufp = swap;
        if (gMultiSioArea.sendFlag)
        {
            swap = gMultiSioArea.currentSendBufp;
            gMultiSioArea.currentSendBufp = gMultiSioArea.nextSendBufp;
            gMultiSioArea.nextSendBufp = swap;
            gMultiSioArea.sendFlag = FALSE;
            gMultiSioArea.sendBufCounter = 0;
        }
        REG_SIOMULTICNT.data = MULTI_SIO_SYNC_DATA;
        gMultiSioArea.hardError = REG_SIOMULTICNT.error;
        REG_SIOCNT |= SIO_START;
        REG_TM3CNT_H = TIMER_ENABLE | TIMER_INTR_ENABLE;
    }
}

void MultiSioIntr(void)
{
    u16 recv[MULTI_SIO_PLAYERS_MAX];
    struct MultiSioPacket *swap;
    s32 i;

    *(u64 *)recv = *(u64 *)&REG_SIOMULTI0;
    gMultiSioArea.hardError = REG_SIOMULTICNT.error;
    if (recv[0] == MULTI_SIO_SYNC_DATA && gMultiSioArea.recvBufCounter > 9)
    {
        gMultiSioArea.recvBufCounter = -1;
        swap = gMultiSioArea.lastRecvBufp;
        gMultiSioArea.lastRecvBufp = gMultiSioArea.currentRecvBufp;
        gMultiSioArea.currentRecvBufp = swap;
        if (gMultiSioArea.sendFlag)
        {
            swap = gMultiSioArea.currentSendBufp;
            gMultiSioArea.currentSendBufp = gMultiSioArea.nextSendBufp;
            gMultiSioArea.nextSendBufp = swap;
            gMultiSioArea.sendFlag = FALSE;
            gMultiSioArea.sendBufCounter = 0;
        }
        REG_IME = 0;
        INTR_CHECK |= INTR_FLAG_SERIAL;
        REG_IME = 1;
    }
    if (gMultiSioArea.sendBufCounter <= 9)
        REG_SIOMULTICNT.data = ((u16 *)gMultiSioArea.currentSendBufp)[gMultiSioArea.sendBufCounter];
    if (gMultiSioArea.sendBufCounter <= 10)
        gMultiSioArea.sendBufCounter++;
    if (gMultiSioArea.recvBufCounter >= 0)
    {
        for (i = 0; i < MULTI_SIO_PLAYERS_MAX; i++)
            ((u16 *)&gMultiSioArea.currentRecvBufp[i])[gMultiSioArea.recvBufCounter] = recv[i];
        if (gMultiSioArea.recvBufCounter == 9)
            gMultiSioArea.recvFlag = TRUE;
    }
    if (gMultiSioArea.recvBufCounter <= 10)
        gMultiSioArea.recvBufCounter++;
    if (gMultiSioArea.type)
    {
        if (gMultiSioArea.sendBufCounter <= 10)
        {
            REG_SIOCNT |= SIO_START;
            REG_TM3CNT_H = TIMER_ENABLE | TIMER_INTR_ENABLE;
        }
        else
        {
            REG_TM3CNT_H = 0;
        }
    }
}
