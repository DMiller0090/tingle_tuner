#include "global.h"
#include "crt0.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "heap.h"
#include "joy_com.h"
#include "main.h"

// JOY bus link with the GameCube (docs/protocol.md). JoyComIntr runs from a copy in
// gMain.heap; JoyComInit copies everything from JoyComIntr up to JoyComMain.

void JoyComInit(void *const *buffers, u8 *status, u32 gameCode, u8 numSlots)
{
    u32 size;
    u8 *intrCopy;
    s32 i, n;

    CpuFill32(0, &gJoyCom, sizeof(gJoyCom));
    gJoyCom.active = TRUE;
    gJoyCom.buffers = buffers;
    gJoyCom.status = status;
    gJoyCom.gameCode = gameCode;
    gJoyCom.numSlots = numSlots;
    n = numSlots;
    for (i = 0; i < n; i++)
        *status++ = GBACOM_STATUS_IDLE;

    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_SERIAL;
    intrCopy = HeapAlloc(&gMain.heap.heap, size = (u32)JoyComMain - (u32)JoyComIntr);
    gJoyCom.intrCopy = intrCopy;
    if (intrCopy != NULL)
    {
        CpuFastCopy(JoyComIntr, intrCopy, size);
        gIntrTable[0] = (IntrFunc)(intrCopy + 1);  // Thumb
    }
    REG_IME = 1;

    REG_JOY_TRANS = JOYCOM_IDLE;
    REG_RCNT = RCNT_JOY_BUS;
    REG_JOYCNT = JOYCNT_INTR | JOYCNT_ACK_ALL;
    REG_IF = INTR_FLAG_SERIAL;
    REG_IME = 0;
    REG_IE |= INTR_FLAG_SERIAL;
    REG_IME = 1;
}

void JoyComStop(void)
{
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_SERIAL;
    REG_IME = 1;
    if (gJoyCom.intrCopy != NULL)
    {
        REG_IME = 0;
        gIntrTable[0] = JoyComIntr;
        REG_IME = 1;
        HeapFree(&gMain.heap.heap, gJoyCom.intrCopy);
        gJoyCom.intrCopy = NULL;
    }
    REG_RCNT = 0;
    REG_JOYCNT = JOYCNT_ACK_ALL;
    REG_JOY_TRANS = 0;
    gJoyCom.active = FALSE;
    gJoyCom.state = JOYCOM_STATE_IDLE;
}

void JoyComIntr(void)
{
    u16 cnt;
    u32 recv;
    struct JoyComPacket *pkt;
    u8 unused[4];  // unused; agbcc still gives it the original's 4-byte stack slot (likely authentic)
    s32 i;

    cnt = REG_JOYCNT;
    REG_JOYCNT = cnt;
    recv = REG_JOY_RECV;
    gJoyCom.closed = FALSE;

    if (cnt & JOYCNT_RESET)
    {
        if (++gJoyCom.resetCount > 9)
        {
            gJoyCom.hardReset = TRUE;
            return;
        }
        gJoyCom.state = JOYCOM_STATE_RESET;
        REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
        REG_JOYSTAT |= JOYSTAT_GP_FLAG1;
        REG_JOY_TRANS = gJoyCom.gameCode;
        gJoyCom.recvState = JOYCOM_WORD_START;
        gJoyCom.sendState = JOYCOM_WORD_START;
        return;
    }

    if (++gJoyCom.otherIntrCount > 3)
    {
        gJoyCom.resetCount = 0;
        gJoyCom.otherIntrCount = 0;
    }

    switch (gJoyCom.state)
    {
    case JOYCOM_STATE_RESET:
        if (cnt & JOYCNT_SEND)
            gJoyCom.state = JOYCOM_STATE_CODE_SENT;
        if (cnt & JOYCNT_RECV)
        {
            REG_JOY_TRANS = JOYCOM_IDLE;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG1;
            gJoyCom.state = JOYCOM_STATE_IDLE;
        }
        break;
    case JOYCOM_STATE_CODE_SENT:
        if ((cnt & JOYCNT_RECV) && recv == gJoyCom.gameCode)
        {
            REG_JOYSTAT |= JOYSTAT_GP_FLAG0;
            gJoyCom.state = JOYCOM_STATE_CODE_OK;
        }
        if (cnt & JOYCNT_SEND)
        {
            REG_JOY_TRANS = JOYCOM_IDLE;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG1;
            gJoyCom.state = JOYCOM_STATE_IDLE;
        }
        break;
    case JOYCOM_STATE_CODE_OK:
        if (cnt & JOYCNT_RECV)
        {
            if (gJoyCom.hasContext)
            {
                if (recv != gJoyCom.context)
                {
                    // Not the GBA the GC linked with before.
                    REG_JOY_TRANS = JOYCOM_IDLE;
                    REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
                    REG_JOYSTAT &= ~JOYSTAT_GP_FLAG1;
                    gJoyCom.state = JOYCOM_STATE_IDLE;
                    break;
                }
            }
            else
            {
                gJoyCom.context = recv;
            }
            // FAKEMATCH: the original reads context through its address (a sym+4 pool
            // constant that the rest of the block is then addressed from); `*&` or this
            // cast gets that, `gJoyCom.context`, `recv` and a `u32 *` local don't (a local
            // assigned first loads its address before REG_JOY_TRANS's).
            REG_JOY_TRANS = *(u32 *)&gJoyCom.context;
            gJoyCom.hasContext = TRUE;
            gJoyCom.state = JOYCOM_STATE_LINKED;
            for (i = 0; i < gJoyCom.numSlots; i++)
                gJoyCom.status[i] = GBACOM_STATUS_IDLE;
            gJoyCom.queueCount = 0;
            gJoyCom.queueWrite = 0;
            gJoyCom.queueRead = 0;
        }
        break;
    case JOYCOM_STATE_LINKED:
        if (cnt & JOYCNT_RECV)
        {
            switch (gJoyCom.recvState)
            {
            case JOYCOM_WORD_START:
                if (recv == JOYCOM_PACKET_START)
                    gJoyCom.recvState = JOYCOM_WORD_TYPE;
                else if (recv == JOYCOM_CLOSE)
                    gJoyCom.closed = TRUE;
                break;
            case JOYCOM_WORD_TYPE:
                gJoyCom.recvType = recv;
                gJoyCom.recvPtr = gJoyCom.buffers[recv];
                gJoyCom.recvState = JOYCOM_WORD_STATUS;
                break;
            case JOYCOM_WORD_STATUS:
                gJoyCom.status[gJoyCom.recvType] = recv;
                if (recv == GBACOM_STATUS_RECEIVING)
                    gJoyCom.recvState = recv;
                else
                    gJoyCom.recvState = JOYCOM_WORD_START;
                break;
            case JOYCOM_WORD_SIZE:
                gJoyCom.recvEnd = (u32 *)((u8 *)gJoyCom.buffers[gJoyCom.recvType] + recv);
                gJoyCom.recvSum = 0;
                gJoyCom.recvState = JOYCOM_WORD_DATA;
                break;
            case JOYCOM_WORD_CHECKSUM:
                gJoyCom.recvCheckSum = recv;
                if (gJoyCom.recvSum == recv)
                {
                    gJoyCom.status[gJoyCom.recvType] = GBACOM_STATUS_RECEIVED;
                }
                else
                {
                    gJoyCom.status[gJoyCom.recvType] = GBACOM_STATUS_ERROR;
                    JoyComStatusSendSet(gJoyCom.recvType);
                }
                gJoyCom.recvState = JOYCOM_WORD_START;
                break;
            case JOYCOM_WORD_DATA:
                *gJoyCom.recvPtr++ = recv;
                gJoyCom.recvSum += recv;
                if (gJoyCom.recvPtr >= gJoyCom.recvEnd)
                    gJoyCom.recvState = JOYCOM_WORD_CHECKSUM;
                break;
            }
        }

        // Load the next word only once the GC has read the previous one.
        if (!(REG_JOYSTAT & JOYSTAT_SEND) && gJoyCom.queueCount != 0)
        {
            pkt = &gJoyCom.queue[gJoyCom.queueRead];
            switch (gJoyCom.sendState)
            {
            case JOYCOM_WORD_START:
                REG_JOY_TRANS = JOYCOM_PACKET_START;
                gJoyCom.sendState = JOYCOM_WORD_TYPE;
                break;
            case JOYCOM_WORD_TYPE:
                REG_JOY_TRANS = pkt->type;
                gJoyCom.sendState = JOYCOM_WORD_STATUS;
                break;
            case JOYCOM_WORD_STATUS:
                if (gJoyCom.status[pkt->type] == GBACOM_STATUS_QUEUED)
                {
                    gJoyCom.status[pkt->type] = GBACOM_STATUS_SENDING;
                    REG_JOY_TRANS = GBACOM_STATUS_SENDING;
                    gJoyCom.sendState = JOYCOM_WORD_SIZE;
                    break;
                }
                // No data: a status packet ends here.
                REG_JOY_TRANS = gJoyCom.status[pkt->type];
                if (gJoyCom.status[pkt->type] == GBACOM_STATUS_REQUEST)
                    gJoyCom.status[pkt->type] = GBACOM_STATUS_REQUESTED;
                goto next;
            case JOYCOM_WORD_SIZE:
                REG_JOY_TRANS = pkt->size;
                pkt->checkSum = 0;
                gJoyCom.sendState = JOYCOM_WORD_DATA;
                break;
            case JOYCOM_WORD_CHECKSUM:
                REG_JOY_TRANS = pkt->checkSum;
                gJoyCom.status[pkt->type] = GBACOM_STATUS_SENT;
            next:
                gJoyCom.queueCount--;
                gJoyCom.queueRead++;
                gJoyCom.sendState = JOYCOM_WORD_START;
                break;
            case JOYCOM_WORD_DATA:
                REG_JOY_TRANS = *pkt->data;
                pkt->checkSum += *pkt->data;
                pkt->size -= 4;
                if (pkt->size <= 0)
                    gJoyCom.sendState = JOYCOM_WORD_CHECKSUM;
                else
                    pkt->data++;
                break;
            }
        }
        break;
    }
    gJoyCom.idleFrames = 0;
}

bool32 JoyComMain(void)
{
    if (gJoyCom.hardReset)
    {
        JoyHardReset();
        JoyComStop();
        return TRUE;
    }
    if (gJoyCom.state == JOYCOM_STATE_LINKED)
    {
        if (gJoyCom.idleFrames > 9)
        {
            // The GC went quiet: restart the link, keeping the context so it can reconnect.
            void *intrCopy;
            u32 context;
            void *const *buffers;
            u8 *status;
            u32 gameCode;
            u8 numSlots;
            u8 hasContext;
            s32 i, n;

            REG_IME = 0;
            REG_IE &= ~INTR_FLAG_SERIAL;
            REG_IME = 1;
            REG_RCNT = 0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG1;
            REG_JOY_TRANS = JOYCOM_IDLE;
            REG_JOYCNT = JOYCNT_INTR | JOYCNT_ACK_ALL;
            REG_RCNT = RCNT_JOY_BUS;

            intrCopy = gJoyCom.intrCopy;
            context = gJoyCom.context;
            buffers = gJoyCom.buffers;
            status = gJoyCom.status;
            gameCode = gJoyCom.gameCode;
            numSlots = gJoyCom.numSlots;
            hasContext = gJoyCom.hasContext;
            CpuFill32(0, &gJoyCom, sizeof(gJoyCom));
            gJoyCom.active = TRUE;
            gJoyCom.intrCopy = intrCopy;
            gJoyCom.context = context;
            gJoyCom.buffers = buffers;
            gJoyCom.status = status;
            gJoyCom.gameCode = gameCode;
            gJoyCom.numSlots = numSlots;
            gJoyCom.hasContext = hasContext;
            n = numSlots;
            for (i = 0; i < n; i++)
                *status++ = GBACOM_STATUS_IDLE;

            REG_IF = INTR_FLAG_SERIAL;
            REG_IME = 0;
            REG_IE |= INTR_FLAG_SERIAL;
            REG_IME = 1;
            gJoyCom.idleFrames = 0;
            return TRUE;
        }
        gJoyCom.idleFrames++;
    }
    return FALSE;
}

bool32 JoyComSendDataSet(void *data, s32 size, u8 type, u32 checkSum)
{
    u32 i;

    if (gJoyCom.queueCount >= JOYCOM_QUEUE_LEN || size <= 0)
        return FALSE;
    if (size < 4)
        size = 4;
    i = gJoyCom.queueWrite;
    gJoyCom.queue[i].data = data;
    gJoyCom.queue[i].size = size;
    gJoyCom.queue[i].type = type;
    gJoyCom.queue[i].checkSum = checkSum;
    gJoyCom.queueWrite++;
    gJoyCom.queueCount++;
    gJoyCom.status[type] = GBACOM_STATUS_QUEUED;
    return TRUE;
}

bool32 JoyComStatusSendSet(u8 type)
{
    u32 i;

    if (gJoyCom.queueCount >= JOYCOM_QUEUE_LEN)
        return FALSE;
    i = gJoyCom.queueWrite;
    gJoyCom.queue[i].data = NULL;
    gJoyCom.queue[i].size = 0;
    gJoyCom.queue[i].type = type;
    gJoyCom.queue[i].checkSum = 0;
    gJoyCom.queueWrite++;
    gJoyCom.queueCount++;
    return TRUE;
}
